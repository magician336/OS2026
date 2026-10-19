#include <slub.h>
#include <pmm.h>
#include <stdio.h>
#include <string.h>

#define OBJECT_SIZE 64
#define OBJECT_ALIGN 16

struct slab {
    struct Page *page;       // 实际后端页面对应的描述符
    struct slab *next;       // 下一个 slab

    void *objects;          // 第一个对象的地址
    void *freelist;         // 第一个空闲对象的地址

    size_t capacity;        // 总对象数
    size_t inuse;           // 已分配对象数

    uint64_t allocated;     // 每一位记录一个对象是否已分配
};

struct kmem_cache {
    size_t stride;          // 相邻对象之间的距离
    struct slab *slabs;
};

static struct kmem_cache cache64;
static bool initialized;

void slub_init(void) {
    if (initialized) {
        return;
    }
    cache64.stride = OBJECT_SIZE;
    cache64.slabs = NULL;
    initialized = 1;
}

static struct slab *new_slab(void) {
    struct Page *page = alloc_page();
    if (page == NULL) {
        return NULL;
    }

    // Page 是描述符，先转换到实际页面的内核虚拟地址。
    char *base = (char *)(page2pa(page) + va_pa_offset);
    struct slab *slab = (struct slab *)base;
    uintptr_t start = ROUNDUP((uintptr_t)base + sizeof(*slab),
                              OBJECT_ALIGN);

    slab->page = page;
    slab->next = NULL;
    slab->objects = (void *)start;
    slab->capacity = (PGSIZE - (start - (uintptr_t)base)) / cache64.stride;
    slab->inuse = 0;
    slab->allocated = 0;
    slab->freelist = slab->objects;
    assert(slab->capacity > 0 && slab->capacity <= 64);

    // 空闲对象的开头保存下一个空闲对象的指针。
    for (size_t i = 0; i < slab->capacity; i++) {
        void *obj = (char *)slab->objects + i * cache64.stride;
        void *next = i + 1 < slab->capacity
                         ? (char *)obj + cache64.stride
                         : NULL;
        *(void **)obj = next;
    }

    return slab;
}

// 仅接受有效对象的起始地址，不接受对象内部地址或 slab 元数据地址。
static bool slab_object_index(const struct slab *slab, const void *ptr,
                              size_t *index) {
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)slab->objects;
    if (addr < start) {
        return 0;
    }

    size_t offset = addr - start;
    if (offset >= slab->capacity * cache64.stride ||
        offset % cache64.stride != 0) {
        return 0;
    }

    *index = offset / cache64.stride;
    return 1;
}

void *kmalloc(size_t size) {
    if (!initialized || size == 0 || size > OBJECT_SIZE) {
        return NULL;
    }

    struct slab *slab = cache64.slabs;
    while (slab != NULL && slab->freelist == NULL) {
        assert(slab->inuse == slab->capacity);
        slab = slab->next;
    }

    if (slab == NULL) {
        slab = new_slab();
        if (slab == NULL) {
            return NULL;
        }
        slab->next = cache64.slabs;
        cache64.slabs = slab;
    }

    void *obj = slab->freelist;
    size_t index;
    assert(slab_object_index(slab, obj, &index));
    uint64_t bit = 1ULL << index;
    assert((slab->allocated & bit) == 0);
    assert(slab->inuse < slab->capacity);

    slab->freelist = *(void **)obj;
    slab->allocated |= bit;
    slab->inuse++;
    return obj;
}

void kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    struct slab **link = &cache64.slabs;
    while (*link != NULL) {
        struct slab *slab = *link;
        size_t index;
        if (!slab_object_index(slab, ptr, &index)) {
            link = &slab->next;
            continue;
        }

        uint64_t bit = 1ULL << index;
        assert((slab->allocated & bit) != 0);
        assert(slab->inuse > 0);

        slab->allocated &= ~bit;
        slab->inuse--;
        *(void **)ptr = slab->freelist;
        slab->freelist = ptr;

        if (slab->inuse == 0) {
            // slab 元数据也在这一页中，归还页面后不能再读取它。
            struct Page *page = slab->page;
            *link = slab->next;
            free_page(page);
        }
        return;
    }

    panic("kfree: pointer does not belong to a live slab object");
}

void slub_check(void) {
    assert(initialized && cache64.slabs == NULL);
    size_t before = nr_free_pages();
    assert(before >= 2);

    assert(kmalloc(0) == NULL);
    assert(kmalloc(OBJECT_SIZE + 1) == NULL);
    kfree(NULL);
    assert(nr_free_pages() == before);

    void *a = kmalloc(1);
    void *b = kmalloc(OBJECT_SIZE);
    assert(a != NULL && b != NULL && a != b);
    assert((uintptr_t)a % OBJECT_ALIGN == 0);
    assert((uintptr_t)b % OBJECT_ALIGN == 0);
    memset(a, 0x11, OBJECT_SIZE);
    memset(b, 0x22, OBJECT_SIZE);

    // 有另一个活跃对象时，释放对象应进入空闲链表并被复用。
    kfree(a);
    void *c = kmalloc(OBJECT_SIZE);
    assert(c == a);
    memset(c, 0x33, OBJECT_SIZE);
    for (size_t i = 0; i < OBJECT_SIZE; i++) {
        assert(((unsigned char *)b)[i] == 0x22);
    }
    kfree(b);
    kfree(c);
    assert(cache64.slabs == NULL);
    assert(nr_free_pages() == before);
    cprintf("[SLUB] 64-byte allocation/reuse/reclaim: PASS\n");

    // 65 个对象超过一页容量，验证跨 slab、全部请求大小及乱序释放。
    void *objects[PGSIZE / OBJECT_SIZE + 1];
    size_t count = sizeof(objects) / sizeof(objects[0]);
    for (size_t i = 0; i < count; i++) {
        objects[i] = kmalloc(i % OBJECT_SIZE + 1);
        assert(objects[i] != NULL);
        assert((uintptr_t)objects[i] % OBJECT_ALIGN == 0);
        for (size_t j = 0; j < i; j++) {
            assert(objects[i] != objects[j]);
        }
        memset(objects[i], (int)(i + 1), OBJECT_SIZE);
    }
    assert(nr_free_pages() == before - 2);

    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < OBJECT_SIZE; j++) {
            assert(((unsigned char *)objects[i])[j] == (unsigned char)(i + 1));
        }
    }
    for (size_t phase = 0; phase < 2; phase++) {
        for (size_t i = phase; i < count; i += 2) {
            kfree(objects[i]);
        }
    }
    assert(cache64.slabs == NULL);
    assert(nr_free_pages() == before);
    cprintf("[SLUB] size boundaries/alignment/cross-slab/data integrity: PASS\n");
}
