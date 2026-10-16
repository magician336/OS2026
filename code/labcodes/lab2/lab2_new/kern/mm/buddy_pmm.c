#include <buddy_pmm.h>
#include <list.h>
#include <stdio.h>
#include <string.h>

/* The default QEMU memory has 32768 pages, so orders 0..15 are sufficient. */
#define BUDDY_MAX_ORDER 16

static list_entry_t free_lists[BUDDY_MAX_ORDER];
static size_t buddy_nr_free;
static struct Page *buddy_base;
static size_t buddy_pages;

static unsigned int order_for_pages(size_t n) {
    unsigned int order = 0;
    size_t size = 1;
    while (size < n) {
        size <<= 1;
        order++;
    }
    return order;
}

static void add_block(struct Page *base, unsigned int order) {
    base->property = order;
    SetPageProperty(base);
    list_add(&free_lists[order], &base->page_link);
}

static void remove_block(struct Page *base) {
    ClearPageProperty(base);
    list_del(&base->page_link);
}

static void buddy_init(void) {
    for (unsigned int order = 0; order < BUDDY_MAX_ORDER; order++) {
        list_init(&free_lists[order]);
    }
    buddy_nr_free = 0;
    buddy_base = NULL;
    buddy_pages = 0;
}

static void buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    buddy_base = base;
    buddy_pages = n;
    buddy_nr_free = n;

    for (struct Page *p = base; p != base + n; p++) {
        assert(PageReserved(p));
        p->flags = 0;
        p->property = 0;
        set_page_ref(p, 0);
        list_init(&p->page_link);
    }

    /* Decompose a possibly unaligned initial interval into buddy blocks. */
    size_t offset = 0;
    while (offset < n) {
        size_t remaining = n - offset;
        unsigned int order = 0;
        size_t block = 1;
        while (order + 1 < BUDDY_MAX_ORDER &&
               block * 2 <= remaining &&
               (offset & (block * 2 - 1)) == 0) {
            block <<= 1;
            order++;
        }
        add_block(base + offset, order);
        offset += block;
    }
}

static struct Page *buddy_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > buddy_nr_free || buddy_base == NULL) {
        return NULL;
    }

    unsigned int wanted = order_for_pages(n);
    if (wanted >= BUDDY_MAX_ORDER) {
        return NULL;
    }

    unsigned int order = wanted;
    while (order < BUDDY_MAX_ORDER && list_empty(&free_lists[order])) {
        order++;
    }
    if (order == BUDDY_MAX_ORDER) {
        return NULL;
    }

    struct Page *page = le2page(list_next(&free_lists[order]), page_link);
    remove_block(page);
    while (order > wanted) {
        order--;
        size_t half = (size_t)1 << order;
        add_block(page + half, order);
    }

    size_t allocated = (size_t)1 << wanted;
    buddy_nr_free -= allocated;
    for (size_t i = 0; i < allocated; i++) {
        page[i].flags = 0;
        page[i].property = 0;
        set_page_ref(&page[i], 0);
        list_init(&page[i].page_link);
    }
    /* Keep the order on the allocated head; free_pages can then accept n. */
    page->property = wanted + 1;
    return page;
}

static void buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    assert(base >= buddy_base && base < buddy_base + buddy_pages);
    unsigned int order = base->property - 1;
    assert(order < BUDDY_MAX_ORDER);
    size_t block = (size_t)1 << order;
    assert(n <= block);
    assert(base + block <= buddy_base + buddy_pages);

    for (size_t i = 0; i < block; i++) {
        assert(!PageReserved(base + i));
        (base + i)->flags = 0;
        (base + i)->property = 0;
        set_page_ref(base + i, 0);
        list_init(&(base + i)->page_link);
    }
    buddy_nr_free += block;

    while (order + 1 < BUDDY_MAX_ORDER) {
        uintptr_t index = (uintptr_t)(base - buddy_base);
        uintptr_t buddy_index = index ^ ((uintptr_t)1 << order);
        if (buddy_index >= buddy_pages ||
            buddy_index + ((size_t)1 << order) > buddy_pages) {
            break;
        }
        struct Page *buddy = buddy_base + buddy_index;
        if (!PageProperty(buddy) || buddy->property != order) {
            break;
        }
        remove_block(buddy);
        if (buddy < base) {
            base = buddy;
        }
        order++;
    }
    add_block(base, order);
}

static size_t buddy_nr_free_pages(void) {
    return buddy_nr_free;
}

static void buddy_check(void) {
    struct Page *real_base = buddy_base;
    size_t real_pages = buddy_pages;
    struct Page sample[16];

    for (size_t i = 0; i < 16; i++) {
        sample[i].flags = 0;
        SetPageReserved(&sample[i]);
    }
    buddy_init();
    buddy_init_memmap(sample, 16);

    struct Page *four_a = buddy_alloc_pages(3);
    struct Page *four_b = buddy_alloc_pages(4);
    struct Page *eight = buddy_alloc_pages(8);
    assert(four_a == sample);
    assert(four_b == sample + 4);
    assert(eight == sample + 8);
    assert(buddy_nr_free == 0);
    assert(buddy_alloc_pages(1) == NULL);

    buddy_free_pages(four_a, 3);
    buddy_free_pages(four_b, 4);
    assert(PageProperty(sample) && sample->property == 3);
    buddy_free_pages(eight, 8);
    assert(PageProperty(sample) && sample->property == 4);
    assert(buddy_nr_free == 16);

    assert(buddy_alloc_pages(16) == sample);
    buddy_free_pages(sample, 16);
    assert(buddy_nr_free == 16);
    cprintf("[Buddy Test] split/order/failure/reuse/merge: PASS\n");

    /* Restore the real page interval after the isolated descriptor test. */
    for (size_t i = 0; i < real_pages; i++) {
        real_base[i].flags = 0;
        SetPageReserved(&real_base[i]);
    }
    buddy_init();
    buddy_init_memmap(real_base, real_pages);
    cprintf("buddy_check() succeeded!\n");
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};
