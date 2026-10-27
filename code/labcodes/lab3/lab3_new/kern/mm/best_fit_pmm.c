#include <pmm.h>
#include <list.h>
#include <best_fit_pmm.h>
#include <stdio.h>

//lab2 function2-2 YOUR CODE
static free_area_t free_area;
#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

static size_t
best_fit_nr_free_pages(void) {
    return nr_free;
}

static void best_fit_init(void){
    list_init(&free_list);
    nr_free = 0;
}

static void
    insert_free_block(struct Page* base){
        if (list_empty(&free_list)) {
            list_add(&free_list, &base->page_link);
            return;
        }
        list_entry_t *le = &free_list;
        while((le=list_next(le)) != &free_list){
            struct Page *p = le2page(le, page_link);
            if (base < p) {
                list_add(le->prev, &base->page_link);
                return;
            }
            if((list_next(le) == &free_list) && (base > p)){
                list_add(le, &base->page_link);
                return;
            }
        }

    }

static void
    best_fit_init_memmap(struct Page *base, size_t n) {
        assert(n > 0);
        for(struct Page *p = base; p != base + n; p++){
           assert(PageReserved(p));
           p->flags=0;
           p->property=0;
           p->ref=0;
            list_init(&p->page_link);
        }
        base->property = n;
        SetPageProperty(base);
        nr_free+=n;
        insert_free_block(base);

    }

static struct Page *
    best_fit_alloc_pages(size_t n) {
        if(n>nr_free){
            return NULL;
        }
        assert(n > 0);
        list_entry_t *le = &free_list;
        struct Page *best_fit = NULL;
        while((le=list_next(le)) != &free_list){
            struct Page *p = le2page(le, page_link);
            if(p->property >= n){
                if(best_fit == NULL || p->property < best_fit->property){
                    best_fit = p;
                }
            }
        }
        if(best_fit == NULL){
            return NULL;
        }
        size_t old_size=best_fit->property;
        list_del(&best_fit->page_link);
        ClearPageProperty(best_fit);
        best_fit->property = 0;
        if(old_size>n){
            struct Page *remain=best_fit+n;
            remain->flags=0;
            remain->property=old_size-n;
            SetPageProperty(remain);
            list_init(&remain->page_link);
            insert_free_block(remain);
        }
        nr_free-=n;
        return best_fit;
    }

static void
    best_fit_free_pages(struct Page *base, size_t n) {
        assert(n > 0);

for (struct Page *p = base; p != base + n; p++) {
    assert(!PageReserved(p) && !PageProperty(p));
    p->flags = 0;
    p->property = 0;
    set_page_ref(p, 0);
    list_init(&p->page_link);
}

base->property = n;
SetPageProperty(base);
nr_free += n;
insert_free_block(base);

list_entry_t *le = list_prev(&base->page_link);
if (le != &free_list) {
    struct Page *prev = le2page(le, page_link);
    if (prev + prev->property == base) {
        prev->property += base->property;
        ClearPageProperty(base);
        list_del(&base->page_link);
        base = prev;
    }
}

le = list_next(&base->page_link);
if (le != &free_list) {
    struct Page *next = le2page(le, page_link);
    if (base + base->property == next) {
        base->property += next->property;
        ClearPageProperty(next);
        list_del(&next->page_link);
    }
}
    }

// Framework tests: students must not modify basic_check or best_fit_check.
static void
basic_check(void) {
    struct Page *p0, *p1, *p2;
    p0 = p1 = p2 = NULL;
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(p0 != p1 && p0 != p2 && p1 != p2);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

    assert(page2pa(p0) < npage * PGSIZE);
    assert(page2pa(p1) < npage * PGSIZE);
    assert(page2pa(p2) < npage * PGSIZE);

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    assert(alloc_page() == NULL);

    free_page(p0);
    free_page(p1);
    free_page(p2);
    assert(nr_free == 3);

    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(alloc_page() == NULL);

    free_page(p0);
    assert(!list_empty(&free_list));

    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    free_list = free_list_store;
    nr_free = nr_free_store;

    free_page(p);
    free_page(p1);
    free_page(p2);
}

static void
best_fit_check(void) {
    int score = 0, sumscore = 6;
    int count = 0, total = 0;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        count++, total += p->property;
    }
    assert(total == nr_free_pages());

    basic_check();

    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n", score, sumscore);
    #endif
    struct Page *p0 = alloc_pages(5), *p1, *p2;
    assert(p0 != NULL);
    assert(!PageProperty(p0));

    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n", score, sumscore);
    #endif
    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));
    assert(alloc_page() == NULL);

    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n", score, sumscore);
    #endif
    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    // * - - * -
    free_pages(p0 + 1, 2);
    free_pages(p0 + 4, 1);
    assert(alloc_pages(4) == NULL);
    assert(PageProperty(p0 + 1) && p0[1].property == 2);
    // * - - * *
    assert((p1 = alloc_pages(1)) != NULL);
    assert(alloc_pages(2) != NULL);
    assert(p0 + 4 == p1);

    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n", score, sumscore);
    #endif
    p2 = p0 + 1;
    free_pages(p0, 5);
    assert((p0 = alloc_pages(5)) != NULL);
    assert(alloc_page() == NULL);

    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n", score, sumscore);
    #endif
    assert(nr_free == 0);
    nr_free = nr_free_store;

    free_list = free_list_store;
    free_pages(p0, 5);

    le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        count--, total -= p->property;
    }
    assert(count == 0);
    assert(total == 0);
    #ifdef ucore_test
    score += 1;
    cprintf("grading: %d / %d points\n", score, sumscore);
    #endif
}

// 附加测试使用独立描述符，暂存并恢复真实空闲区；不改动框架检查函数。
static void extra_check_list(void) {
    size_t total = 0, blocks = 0;
    struct Page *previous = NULL;
    for (list_entry_t *le = list_next(&free_list); le != &free_list;
         le = list_next(le)) {
        struct Page *page = le2page(le, page_link);
        assert(PageProperty(page) && page->property > 0);
        assert(list_prev(list_next(le)) == le);
        assert(list_next(list_prev(le)) == le);
        if (previous != NULL) {
            assert(previous + previous->property < page);
        }
        previous = page;
        total += page->property;
        assert(++blocks <= 40);
    }
    assert(total == nr_free);
}

static void extra_reset(struct Page *sample, size_t n) {
    best_fit_init();
    for (size_t i = 0; i < n; i++) {
        sample[i].flags = 0;
        SetPageReserved(&sample[i]);
    }
    best_fit_init_memmap(sample, n);
    assert(best_fit_alloc_pages(n) == sample);
    assert(nr_free == 0);
}

static void best_fit_extra_check(void) {
    free_area_t saved = free_area;
    struct Page sample[40];
    extra_reset(sample, 40);
    best_fit_free_pages(sample + 1, 3);
    best_fit_free_pages(sample + 6, 5);
    best_fit_free_pages(sample + 12, 8);
    best_fit_free_pages(sample + 24, 5);
    extra_check_list();
    assert(nr_free == 21);
    // 总页数够用，但最大的连续块只有8页。
    assert(best_fit_alloc_pages(9) == NULL);
    assert(best_fit_alloc_pages(22) == NULL);
    assert(nr_free == 21);

    // 5页块优于8页块；同尺寸时选择地址较低的5页块。
    assert(best_fit_alloc_pages(4) == sample + 6);
    assert(PageProperty(sample + 10) && sample[10].property == 1);
    assert(best_fit_alloc_pages(3) == sample + 1);
    assert(best_fit_alloc_pages(5) == sample + 24);
    assert(best_fit_alloc_pages(2) == sample + 12);
    assert(nr_free == 7);
    assert(best_fit_alloc_pages(7) == NULL);
    assert(nr_free == 7);
    extra_check_list();

    extra_reset(sample, 40);
    best_fit_free_pages(sample + 5, 3);
    best_fit_free_pages(sample + 10, 4);
    best_fit_free_pages(sample + 8, 2); // 同时连接前后空闲块。
    assert(nr_free == 9 && sample[5].property == 9);
    extra_check_list();
    assert(best_fit_alloc_pages(9) == sample + 5);
    assert(nr_free == 0);

    best_fit_free_pages(sample + 8, 4);
    best_fit_free_pages(sample + 4, 4);  // 仅向后合并。
    assert(sample[4].property == 8);
    best_fit_free_pages(sample + 12, 4); // 仅向前合并。
    assert(sample[4].property == 12 && nr_free == 12);
    extra_check_list();

    free_area = saved;
    assert(list_prev(list_next(&free_list)) == &free_list);
    assert(list_next(list_prev(&free_list)) == &free_list);
    cprintf("[Best-Fit extra] choice/tie/split/fragmentation/merge: PASS\n");
}

static void best_fit_check_all(void) {
    best_fit_check();
    best_fit_extra_check();
}

const struct pmm_manager best_fit_pmm_manager = {
    .name = "best_fit_pmm_manager",
    .init = best_fit_init,
    .init_memmap = best_fit_init_memmap,
    .alloc_pages = best_fit_alloc_pages,
    .free_pages = best_fit_free_pages,
    .nr_free_pages = best_fit_nr_free_pages,
    .check = best_fit_check_all,
};
