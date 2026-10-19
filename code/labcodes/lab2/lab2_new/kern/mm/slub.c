#include <slub.h>
#include <pmm.h>
#include <string.h>

#define MIN_OBJECT_SIZE 16
#define MAX_CACHE_SIZE 2048
#define MAX_OBJECTS (PGSIZE / MIN_OBJECT_SIZE)
#define BITMAP_WORDS ((MAX_OBJECTS + 63) / 64)

struct kmem_cache;

// 元数据在后端页面开头；空闲对象自身保存下一个空闲对象的指针。
struct slab {
    struct Page *page;
    size_t npages;
    struct kmem_cache *cache; // NULL 表示直接分配的大对象
    struct slab *next;
    struct slab *partial_next;
    void *objects;
    void *freelist;
    size_t stride;
    size_t capacity;
    size_t inuse;
    uint64_t allocated[BITMAP_WORDS];
};

struct kmem_cache {
    size_t stride;
    struct slab *slabs;
    struct slab *partial;    // 仅包含仍有空闲对象的 slab
};

static const size_t cache_sizes[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048
};
#define CACHE_COUNT (sizeof(cache_sizes) / sizeof(cache_sizes[0]))

static struct kmem_cache caches[CACHE_COUNT];
static struct slab *large_slabs;
static bool initialized;

void slub_init(void) {
    if (initialized) {
        return;
    }
    for (size_t i = 0; i < CACHE_COUNT; i++) {
        caches[i].stride = cache_sizes[i];
        caches[i].slabs = NULL;
        caches[i].partial = NULL;
    }
    large_slabs = NULL;
    initialized = 1;
}

static struct kmem_cache *find_cache(size_t size, size_t alignment) {
    for (size_t i = 0; i < CACHE_COUNT; i++) {
        if (size <= caches[i].stride && alignment <= caches[i].stride) {
            return &caches[i];
        }
    }
    return NULL;
}

static bool object_index(const struct slab *slab, const void *ptr,
                         size_t *index) {
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)slab->objects;
    if (addr < start) {
        return 0;
    }
    size_t offset = addr - start;
    if (slab->cache == NULL) {
        if (offset != 0) {
            return 0;
        }
        *index = 0;
        return 1;
    }
    if (offset >= slab->capacity * slab->stride ||
        offset % slab->stride != 0) {
        return 0;
    }
    *index = offset / slab->stride;
    return 1;
}

static struct slab *new_slab(struct kmem_cache *cache) {
    struct Page *page = alloc_page();
    if (page == NULL) {
        return NULL;
    }
    // Page 是描述符，不能直接将其地址用作对象存储。
    char *base = (char *)(page2pa(page) + va_pa_offset);
    struct slab *slab = (struct slab *)base;
    memset(slab, 0, sizeof(*slab));
    uintptr_t start = ROUNDUP((uintptr_t)base + sizeof(*slab), cache->stride);
    slab->page = page;
    slab->npages = 1;
    slab->cache = cache;
    slab->stride = cache->stride;
    slab->objects = (void *)start;
    slab->capacity = (PGSIZE - (start - (uintptr_t)base)) / slab->stride;
    assert(slab->capacity > 0 && slab->capacity <= MAX_OBJECTS);
    slab->freelist = slab->objects;

    for (size_t i = 0; i < slab->capacity; i++) {
        void *obj = (char *)slab->objects + i * slab->stride;
        *(void **)obj = i + 1 < slab->capacity
                           ? (char *)obj + slab->stride : NULL;
    }
    slab->next = cache->slabs;
    cache->slabs = slab;
    slab->partial_next = cache->partial;
    cache->partial = slab;
    return slab;
}

static void *alloc_large(size_t size, size_t alignment) {
    // base 按页对齐，alignment <= PGSIZE，所以偏移对齐即可。
    size_t offset = ROUNDUP(sizeof(struct slab), alignment);
    size_t max_size = ~(size_t)0;
    if (size > max_size - offset) {
        return NULL;
    }
    size_t total = offset + size;
    if (total > max_size - (PGSIZE - 1)) {
        return NULL;
    }
    size_t npages = (total + PGSIZE - 1) / PGSIZE;
    if (npages > nr_free_pages()) {
        return NULL;
    }
    struct Page *page = alloc_pages(npages);
    if (page == NULL) {
        return NULL;
    }
    char *base = (char *)(page2pa(page) + va_pa_offset);
    struct slab *slab = (struct slab *)base;
    memset(slab, 0, sizeof(*slab));
    slab->page = page;
    slab->npages = npages;
    slab->objects = base + offset;
    slab->stride = size;
    slab->capacity = slab->inuse = 1;
    slab->allocated[0] = 1;
    slab->next = large_slabs;
    large_slabs = slab;
    return slab->objects;
}

void *kmalloc_aligned(size_t size, size_t alignment) {
    if (!initialized || size == 0 || alignment == 0 ||
        alignment > PGSIZE || (alignment & (alignment - 1)) != 0) {
        return NULL;
    }
    if (alignment < SLUB_DEFAULT_ALIGN) {
        alignment = SLUB_DEFAULT_ALIGN;
    }
    struct kmem_cache *cache = find_cache(size, alignment);
    if (size > MAX_CACHE_SIZE || cache == NULL) {
        return alloc_large(size, alignment);
    }
    struct slab *slab = cache->partial;
    if (slab == NULL) {
        slab = new_slab(cache);
        if (slab == NULL) {
            return NULL;
        }
    }
    void *obj = slab->freelist;
    size_t index;
    assert(object_index(slab, obj, &index));
    size_t word = index / 64;
    uint64_t bit = 1ULL << (index % 64);
    assert((slab->allocated[word] & bit) == 0);
    assert(slab->inuse < slab->capacity);
    slab->freelist = *(void **)obj;
    slab->allocated[word] |= bit;
    slab->inuse++;
    if (slab->freelist == NULL) {
        assert(slab->inuse == slab->capacity);
        cache->partial = slab->partial_next;
        slab->partial_next = NULL;
    }
    return obj;
}

void *kmalloc(size_t size) {
    return kmalloc_aligned(size, SLUB_DEFAULT_ALIGN);
}

static void remove_partial(struct kmem_cache *cache, struct slab *slab) {
    struct slab **link = &cache->partial;
    while (*link != NULL && *link != slab) {
        link = &(*link)->partial_next;
    }
    assert(*link == slab);
    *link = slab->partial_next;
}

void kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    for (size_t i = 0; i < CACHE_COUNT; i++) {
        struct kmem_cache *cache = &caches[i];
        struct slab **link = &cache->slabs;
        while (*link != NULL) {
            struct slab *slab = *link;
            size_t index;
            if (!object_index(slab, ptr, &index)) {
                link = &slab->next;
                continue;
            }
            size_t word = index / 64;
            uint64_t bit = 1ULL << (index % 64);
            if ((slab->allocated[word] & bit) == 0) {
                panic("kfree: object is already free");
            }
            assert(slab->inuse > 0);
            bool was_full = slab->freelist == NULL;
            slab->allocated[word] &= ~bit;
            slab->inuse--;
            *(void **)ptr = slab->freelist;
            slab->freelist = ptr;
            if (slab->inuse == 0) {
                if (!was_full) {
                    remove_partial(cache, slab);
                }
                struct Page *page = slab->page;
                *link = slab->next;
                free_page(page);
            } else if (was_full) {
                slab->partial_next = cache->partial;
                cache->partial = slab;
            }
            return;
        }
    }
    struct slab **link = &large_slabs;
    while (*link != NULL) {
        struct slab *slab = *link;
        if (ptr == slab->objects) {
            struct Page *page = slab->page;
            size_t npages = slab->npages;
            *link = slab->next;
            // Buddy从头页恢复实际阶次，允许按原请求页数归还。
            free_pages(page, npages);
            return;
        }
        link = &slab->next;
    }
    panic("kfree: pointer is not a live object");
}
