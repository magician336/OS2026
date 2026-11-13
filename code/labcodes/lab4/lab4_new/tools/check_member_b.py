#!/usr/bin/env python3
"""Host isolation test of the actual proc.c; no RISC-V execution is claimed."""
import pathlib, re, subprocess, tempfile
root = pathlib.Path(__file__).resolve().parents[1]
def source(path):
    return re.sub(r'^#include.*$', '', (root / path).read_text(), flags=re.M)
pre = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
typedef int bool;
#undef static_assert
#define static_assert(x) _Static_assert(x, #x)
#define to_struct(p,t,m) ((t *)((char *)(p)-offsetof(t,m)))
#define KSTACKSIZE 8192
#define KSTACKPAGE 2
#define CLONE_VM 0x100
#define E_NO_MEM 4
#define E_NO_FREE_PROC 5
#define SSTATUS_SIE 2
#define SSTATUS_SPIE 32
#define SSTATUS_SPP 256
static uintptr_t status = 2, boot_pgdir_pa = 0x80400000;
static char bootstack[KSTACKSIZE];
#define read_csr(x) status
#define local_intr_save(x) do { x=status; status=0; } while(0)
#define local_intr_restore(x) (status=(x))
#define panic(...) abort()
#define cprintf(...) ((void)0)
static int fail_pcb, fail_stack, pcbs, stacks;
static void *kmalloc(size_t n) { if(fail_pcb) return NULL; pcbs++; void *p=malloc(n); memset(p,0xa5,n); return p; }
static void kfree(void *p) { pcbs--; free(p); }
struct Page;
static struct Page *alloc_pages(int n) { assert(n==2); if(fail_stack)return NULL; stacks++; return (void *)aligned_alloc(16,KSTACKSIZE); }
static void free_pages(struct Page *p,int n) { assert(n==2); stacks--; free(p); }
#define page2kva(p) ((void *)(p))
#define kva2page(p) ((struct Page *)(p))
static unsigned hash32(unsigned x,unsigned bits) { return x & ((1U<<bits)-1); }
'''
post = r'''
void kernel_thread_entry(void) {}
void forkrets(struct trapframe *tf) { abort(); }
void wakeup_proc(struct proc_struct *p) {
 assert(p->state==PROC_UNINIT); assert(find_proc(p->pid)==p);
 assert(p->tf && p->parent && p->context.ra==(uintptr_t)forkret);
 p->state=PROC_RUNNABLE;
}
int main(void) {
 struct proc_struct *p=alloc_proc();
 assert(p && p->pid==-1 && p->pgdir==boot_pgdir_pa && p->state==PROC_UNINIT);
 assert(!p->runs && !p->kstack && !p->need_resched && !p->parent && !p->mm && !p->tf && !p->flags);
 struct context zero={0}; assert(!memcmp(&p->context,&zero,sizeof(zero)));
 for(unsigned i=0;i<sizeof(p->name);i++) assert(!p->name[i]);
 assert(list_empty(&p->list_link) && list_empty(&p->hash_link)); kfree(p);
 fail_pcb=1; assert(!alloc_proc()); fail_pcb=0;
 proc_init(); assert(nr_process==2 && pcbs==2 && stacks==1);
 assert(current==idleproc && idleproc->need_resched && idleproc->kstack==(uintptr_t)bootstack);
 assert(initproc->pid==1 && initproc->parent==idleproc && find_proc(1)==initproc);
 assert(initproc->tf->gpr.s0==(uintptr_t)init_main && initproc->tf->epc==(uintptr_t)kernel_thread_entry);
 assert(initproc->tf->status==(SSTATUS_SPP|SSTATUS_SPIE));
 assert(initproc->tf->gpr.a0==0 && initproc->tf->gpr.sp==(uintptr_t)initproc->tf);
 assert((uintptr_t)initproc->tf%16==0 && initproc->context.sp==(uintptr_t)initproc->tf);
 list_entry_t *before=proc_list.next;
 fail_pcb=1; assert(kernel_thread(init_main,0,0)==-E_NO_MEM); fail_pcb=0;
 fail_stack=1; assert(kernel_thread(init_main,0,0)==-E_NO_MEM); fail_stack=0;
 assert(pcbs==2 && stacks==1 && nr_process==2 && proc_list.next==before);
 nr_process=MAX_PROCESS; assert(kernel_thread(init_main,0,0)==-E_NO_FREE_PROC); nr_process=2;
 struct trapframe tf={0}; tf.gpr.a0=99;
 int pid=do_fork(0,0x12340,&tf); p=find_proc(pid);
 assert(pid==2 && p && p->tf->gpr.sp==0x12340 && p->tf->gpr.a0==0 && tf.gpr.a0==99);
 assert(nr_process==3 && pcbs==3 && stacks==2 && status==2);
 puts("member B: PCB, init, publication, trapframe, PID and allocation rollback PASS");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='lab4-b-') as d:
    p=pathlib.Path(d)
    text=pre+source('libs/list.h')+source('kern/trap/trap.h')+source('kern/process/proc.h')
    text+='\nvoid wakeup_proc(struct proc_struct *);\n'+source('kern/process/proc.c')+post
    (p/'test.c').write_text(text)
    subprocess.run(['cc','-std=gnu11','-Wno-unused-value',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
