#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <assert.h>

void
wakeup_proc(struct proc_struct *proc) {
    assert(proc->state != PROC_ZOMBIE && proc->state != PROC_RUNNABLE);
    proc->state = PROC_RUNNABLE;
}

//LAB4 function3 2411041
// schedule - 一次调度决策:从 proc_list 中选出下一个可运行线程并切换过去。
// 轮转起点取 current 之后;idleproc 不在链表中,扫描绕过链表尾时自然退回 idleproc。
// 决策区(扫描、runs 自增)在关中断临界区内完成,避免时钟中断打断选择过程。
void
schedule(void) {
    bool intr_flag;
    struct proc_struct *next;
    local_intr_save(intr_flag);
    {
        next = idleproc;
        list_entry_t *le = (current == idleproc) ? &proc_list : &(current->list_link);
        while ((le = list_next(le)) != &proc_list) {
            struct proc_struct *p = le2proc(le, list_link);
            if (p->state == PROC_RUNNABLE) {
                next = p;
                break;
            }
        }
        next->runs++;
    }
    local_intr_restore(intr_flag);
    if (next != current) {
        proc_run(next);
    }
}
