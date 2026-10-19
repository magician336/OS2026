#include <slub.h>
#include <pmm.h>
#include <stdio.h>
#include <string.h>

static void check_bytes(const void *ptr, size_t size, unsigned char value) {
    const unsigned char *bytes = ptr;
    for (size_t i = 0; i < size; i++) {
        assert(bytes[i] == value);
    }
}

static void check_sizes(void) {
    static const size_t sizes[] = {
        1, 15, 16, 17, 31, 32, 33, 63, 64, 65,
        127, 128, 129, 255, 256, 257, 511, 512, 513,
        1023, 1024, 1025, 2047, 2048, 2049,
        4095, 4096, 4097, 8193, 65537
    };
    void *objects[sizeof(sizes) / sizeof(sizes[0])];
    size_t before = nr_free_pages();
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        objects[i] = kmalloc(sizes[i]);
        assert(objects[i] != NULL);
        assert((uintptr_t)objects[i] % SLUB_DEFAULT_ALIGN == 0);
        for (size_t j = 0; j < i; j++) {
            uintptr_t a = (uintptr_t)objects[i];
            uintptr_t b = (uintptr_t)objects[j];
            assert(a + sizes[i] <= b || b + sizes[j] <= a);
        }
        memset(objects[i], (int)(i + 1), sizes[i]);
    }
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        check_bytes(objects[i], sizes[i], (unsigned char)(i + 1));
    }
    for (size_t phase = 0; phase < 2; phase++) {
        for (size_t i = phase; i < sizeof(sizes) / sizeof(sizes[0]); i += 2) {
            kfree(objects[i]);
        }
    }
    assert(nr_free_pages() == before);
    cprintf("[SLUB] size classes/large objects/data integrity: PASS\n");
}

static void check_reuse(void) {
    size_t before = nr_free_pages();
    void *a = kmalloc(64);
    void *b = kmalloc(64);
    assert(a != NULL && b != NULL && a != b);
    memset(b, 0x22, 64);
    slub_init(); // 重复初始化不能丢失现有分配。
    kfree(a);
    void *c = kmalloc(64);
    assert(c == a);
    memset(c, 0x33, 64);
    check_bytes(b, 64, 0x22);
    kfree(b);
    kfree(c);
    assert(nr_free_pages() == before);

    // 257个16字节对象跨slab，同时覆盖多个位图字。
    void *objects[PGSIZE / 16 + 1];
    size_t count = sizeof(objects) / sizeof(objects[0]);
    for (size_t i = 0; i < count; i++) {
        objects[i] = kmalloc(16);
        assert(objects[i] != NULL);
        for (size_t j = 0; j < i; j++) {
            assert(objects[i] != objects[j]);
        }
        memset(objects[i], (int)(i & 0xff), 16);
    }
    assert(nr_free_pages() <= before - 2);
    for (size_t i = 0; i < count; i++) {
        check_bytes(objects[i], 16, (unsigned char)i);
    }
    void *saved = objects[64];
    kfree(saved);
    objects[64] = kmalloc(16);
    assert(objects[64] == saved);
    for (size_t phase = 0; phase < 2; phase++) {
        for (size_t i = phase; i < count; i += 2) {
            kfree(objects[i]);
        }
    }
    assert(nr_free_pages() == before);

    // 一个2048字节对象占满其slab，覆盖full -> empty的直接回收。
    a = kmalloc(2048);
    assert(a != NULL);
    kfree(a);
    assert(nr_free_pages() == before);
    cprintf("[SLUB] reuse/bitmap/cross-slab/page reclamation: PASS\n");
}

static void check_alignment(void) {
    size_t before = nr_free_pages();
    for (size_t alignment = 1; alignment <= PGSIZE; alignment <<= 1) {
        size_t expected = alignment < SLUB_DEFAULT_ALIGN
                              ? SLUB_DEFAULT_ALIGN : alignment;
        void *small = kmalloc_aligned(37, alignment);
        void *large = kmalloc_aligned(8193, alignment);
        assert(small != NULL && large != NULL);
        assert((uintptr_t)small % expected == 0);
        assert((uintptr_t)large % expected == 0);
        memset(small, 0xa5, 37);
        memset(large, 0x5a, 8193);
        check_bytes(small, 37, 0xa5);
        check_bytes(large, 8193, 0x5a);
        kfree(small);
        kfree(large);
    }
    size_t max_size = ~(size_t)0;
    assert(kmalloc(0) == NULL);
    assert(kmalloc(max_size) == NULL);
    assert(kmalloc(max_size - PGSIZE) == NULL);
    assert(kmalloc_aligned(64, 0) == NULL);
    assert(kmalloc_aligned(64, 3) == NULL);
    assert(kmalloc_aligned(64, PGSIZE * 2) == NULL);
    kfree(NULL);
    assert(nr_free_pages() == before);
    cprintf("[SLUB] alignment/zero/overflow/invalid requests: PASS\n");
}

static void check_exhaustion(void) {
    size_t before = nr_free_pages();
    void *live = kmalloc(64);
    assert(live != NULL);
    memset(live, 0x77, 64);
    list_entry_t held;
    list_init(&held);
    size_t held_count = 0;
    struct Page *page;

    // 真实耗尽后端。已分配页面的page_link用于测试链表，不占额外页面。
    while ((page = alloc_page()) != NULL) {
        list_add(&held, &page->page_link);
        held_count++;
    }
    assert(nr_free_pages() == 0);
    void *reused = kmalloc(64);
    assert(reused != NULL);      // 已有partial slab不需要申请新页。
    assert(kmalloc(16) == NULL); // 新cache无法取得页面。
    assert(kmalloc(8193) == NULL);
    check_bytes(live, 64, 0x77);
    kfree(reused);
    kfree(live);

    size_t restored = 0;
    while (!list_empty(&held)) {
        list_entry_t *entry = list_next(&held);
        page = le2page(entry, page_link);
        list_del(entry);
        free_page(page);
        restored++;
    }
    assert(restored == held_count);
    assert(nr_free_pages() == before);
    void *again = kmalloc(8193);
    assert(again != NULL);
    kfree(again);
    assert(nr_free_pages() == before);
    cprintf("[SLUB] real exhaustion/failure/recovery: PASS\n");
}

void slub_check(void) {
    size_t before = nr_free_pages();
    check_sizes();
    check_reuse();
    check_alignment();
    check_exhaustion();
    assert(nr_free_pages() == before);
    cprintf("[SLUB] all checks passed (%s)\n", pmm_manager->name);

    // 每种非法释放单独启动一次，以预期panic判断测试通过。
#ifdef SLUB_NEGATIVE_TEST
    void *ptr = kmalloc(64);
    void *guard = kmalloc(64);
    assert(ptr != NULL && guard != NULL);
#if SLUB_NEGATIVE_TEST == 1
    kfree((char *)ptr + 1);
#elif SLUB_NEGATIVE_TEST == 2
    kfree(ptr);
    kfree(ptr);
#elif SLUB_NEGATIVE_TEST == 3
    int foreign;
    kfree(&foreign);
#elif SLUB_NEGATIVE_TEST == 4
    void *large = kmalloc(8193);
    assert(large != NULL);
    kfree((char *)large + PGSIZE);
#elif SLUB_NEGATIVE_TEST == 5
    void *large = kmalloc(8193);
    assert(large != NULL);
    kfree(large);
    kfree(large);
#else
#error Unsupported SLUB_NEGATIVE_TEST value
#endif
    panic("SLUB negative test unexpectedly returned");
#endif
}
