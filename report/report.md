# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 4:进程管理 |
| **小组成员** | 2411046-梁家瑞、2412727-刘蔚霖、2411041-李培涛 |
| **完成日期** | 2026-11-13(骨架导入与分工确认);最终完成日期【待填写】 |

### 小组分工

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2411046-梁家瑞(成员 A) | 功能模块一:虚拟内存页操作(`kern/mm/pmm.c` 的 `page_insert()`、`page_remove()`);Challenge 2(`get_pte()` 与 sv32/sv39/sv48 分页模式思考题、查找与分配是否拆分) |
| 2412727-刘蔚霖(成员 B) | 功能模块二:进程创建及初始化(`kern/process/proc.c` 的 `alloc_proc()`、`proc_init()`、`kernel_thread()`、`do_fork()`、`copy_thread()`、`copy_mm()`) |
| 2411041-李培涛(成员 C) | 功能模块三:进程调度(`kern/process/proc.c` 的 `cpu_idle()`、`proc_run()`,`kern/schedule/sched.c` 的 `schedule()`,`kern/process/switch.S` 的 `switch_to()`);Challenge 1(`local_intr_save`/`local_intr_restore` 开关中断机制);接口集成与联调验证(骨架中的 lab2/lab3 迁移由成员 C 预置,复核与 `make qemu`/`make grade` 最终联调归成员 C)、报告汇总交付 |

实验报告分工:各成员撰写本人负责模块的功能描述、最终提示词与实现迭代过程;成员 C 汇总实验整体逻辑分析、测试验证与实验总结。

---

## 一、实验目的

1. 理解虚拟内存管理的基本结构,掌握虚拟内存的组织与管理方式(vmm 与预映射式地址空间)。
2. 理解内核线程创建/执行的管理过程,掌握进程控制块 `struct proc_struct` 的字段含义与生命周期状态。
3. 掌握内核线程的切换和基本调度过程,理解 `context`/`trapframe` 两级现场与 `forkret → forkrets → __trapret` 首次运行链路。
4. 继续使用 AI 辅助开发,记录提示词迭代、实现问题和验证证据。

---

## 二、实验环境

| 成员 | 环境与工具 | AI 编程工具与模型 |
|------|------------|-------------------|
| 成员 A | OS2026 工程、RISC-V 64、`riscv64-unknown-elf-` 工具链、QEMU 4.1.1 | 【待填写】 |
| 成员 B | macOS、`riscv64-elf-gcc 16.2.0`、QEMU 11.1.2 | 【待填写】 |
| 成员 C | Fedora、`riscv64-unknown-elf-gcc 15.1.0`、QEMU 4.1.1(本地 `/opt/qemu-4.1.1`) | 【待填写】 |

代码目录:`code/labcodes/lab4/lab4_new`,目标架构 RISC-V 64。

【待完成:各成员实际使用的 AI 编程工具与底层模型版本,开始实现后据实填写。】

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

【待完成。参考主线:lab4 按"虚拟内存页操作 → 进程创建与初始化 → 进程调度"三条主线展开,执行链为 `kern_init → pmm_init → pic_init/idt_init → vmm_init → proc_init → cpu_idle → schedule → proc_run → switch_to`;前半段把地址空间准备好,后半段让第一个可运行线程真正跑起来。】

### 3.2 功能的逐步实现

【待完成:按三个功能模块的依赖顺序说明为什么先做页操作、再做进程创建、最后做调度。】

---

## 四、实验内容与实现

### 功能模块一:虚拟内存页操作

**负责人:** 2411046-梁家瑞

**需要实现/修改的函数:**

```c
int page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm);(kern/mm/pmm.c)
void page_remove(pde_t *pgdir, uintptr_t la);(kern/mm/pmm.c)
```

**功能说明:** 在现有多级页表框架上完成最基本的映射维护:把物理页挂到指定线性地址上、把已有映射安全撤销。关键约束是保持"页表项内容、物理页引用计数、TLB 状态"三者同步:`page_insert` 需正确处理原地址已映射到同一物理页/其他物理页两种情况,失败时返回 `-E_NO_MEM` 且不留半完成映射;`page_remove` 是幂等删除接口,目标地址无映射时安静返回。依赖 `get_pte()`、`pte_create()`、`pte2page()`、`page_ref_inc()`/`page_ref_dec()`、`set_page_ref()`、`tlb_invalidate()` 等接口与 `PTE_V/PTE_U/PTE_W/PTE_R` 权限位宏。

#### 最终提示词

【待完成】

#### 实现迭代过程

【待完成】

---

### 功能模块二:进程创建及初始化

**负责人:** 2412727-刘蔚霖

**需要实现/修改的函数:**

```c
static struct proc_struct *alloc_proc(void);(kern/process/proc.c)
void proc_init(void);(kern/process/proc.c)
int kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags);(kern/process/proc.c)
int do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf);(kern/process/proc.c)
static void copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf);(kern/process/proc.c)
static int copy_mm(uint32_t clone_flags, struct proc_struct *proc);(kern/process/proc.c)
```

**功能说明:** 把系统从"已经能运行内核代码"推进到"已经能创建并登记多个内核线程"。`alloc_proc` 返回干净 PCB;`proc_init` 把当前执行现场包装成 `idleproc` 并创建第一个内核线程 `initproc`;`kernel_thread` 把函数入口和参数整理成线程首次运行所需的临时上下文;`do_fork` 组合内核栈、PID、链表与状态位等资源并处理失败清理;`copy_thread` 把 trapframe 和 context 安放到新线程内核栈上;`copy_mm` 在本实验承担地址空间处理的占位形态。完成后系统至少存在 `idleproc` 与 `initproc` 两个关键线程。

#### 提示词与实际协作记录

见 [Prompt B](prompt.md#member-b-prompt)。该提示词在实现提交后补充，用于后续手写练习；本次实际由 Codex 实现核心代码，协作过程已在同节说明。

#### 实现迭代过程

成员 B 已实现六个函数，详细验收、调用链、失败回收和复现命令见 [成员 B 实现与验证](report-memberB.md)。提示词及实际协作说明已补充。

`alloc_proc` 清零 PCB（进程控制块），设置未初始化态、无效 PID、共享内核页表物理地址，并初始化两个链表节点。`proc_init` 将启动现场包装成 PID 0 的 idleproc，再通过 kernel_thread 创建 PID 1 的 initproc。`kernel_thread` 依照 entry.S 将函数和参数放入 s0/s1，设置 SPP/SPIE 并清除 SIE。`copy_thread` 将 trapframe（陷阱帧）复制到独立内核栈顶，设置子线程 a0=0 和 forkret 入口。`copy_mm` 明确断言父线程没有用户地址空间。`do_fork` 在资源准备完成后关中断分配 PID、登记哈希/链表、计数并唤醒；分配失败只回收已取得的资源。

实际修正：隔离测试初版的 static_assert 与宿主头文件重名，增加 undef；增强评分断言时补上 riscv.h；注入编译发现原测试线程返回 void 与 kernel_thread 的 int 返回值接口不符，修正测试函数签名（死循环行为不变）。没有修改评分标准或降低断言。


---

### 功能模块三:进程调度

**负责人:** 2411041-李培涛

**需要实现/修改的函数:**

```c
void cpu_idle(void);(kern/process/proc.c)
void schedule(void);(kern/schedule/sched.c)
void proc_run(struct proc_struct *proc);(kern/process/proc.c)
void switch_to(struct context *from, struct context *to);(kern/process/switch.S)
```

**功能说明:** 把已创建的内核线程真正交给 CPU 执行。`cpu_idle` 作为空闲线程主体等待调度需求;`schedule` 从进程链表选出下一个 runnable 线程,找不到时退回 `idleproc`;`proc_run` 在关中断临界区内切换 current、页表（Sv39 的 satp，不能使用遗留 Sv32 `lsatp()`）并调用 `switch_to`;`switch_to` 只做两个 `struct context` 之间的寄存器保存与恢复。切换原子性是本模块核心约束。新线程第一次能走到正确入口,依赖模块二预组织的 `forkret → forkrets → __trapret` 恢复链路。

#### 最终提示词

【待完成】

#### 实现迭代过程

【待完成】

---

### Challenge 1:`local_intr_save`/`local_intr_restore` 开关中断机制

**负责人:** 2411041-李培涛

【待完成:说明该宏对如何借助 `sstatus.SIE` 位与局部变量 `intr_flag` 实现开关中断,以及为什么不能用无条件 `intr_enable()`/`intr_disable()` 替代。】

---

### Challenge 2:`get_pte()` 与多级分页模式思考题

**负责人:** 2411046-梁家瑞

【待完成:结合 sv32/sv39/sv48 三种分页模式的异同,解释 `get_pte()` 中两段形式相似代码的成因;讨论把页表项查找与页表页分配合并在一个函数里是否合适、有无必要拆分。】

---

## 五、测试与验证

### 骨架基线(2026-11-13,成员 C)

上游 lab4 骨架(`code/labcodes/lab4/lab4_new`,68 个文件)导入后,先完成 lab2/lab3 依赖代码迁移再验证构建。骨架共 5 处 lab2/lab3 占位,迁移来源如下:

| 占位位置 | 迁移内容 | 来源 |
|----------|----------|------|
| `kern/mm/default_pmm.c`(`//lab2 请补充`) | best-fit 物理页分配器,按骨架要求以 `default_*` 命名填充并补 `default_pmm_manager` 结构体 | 本组 lab2 `best_fit_pmm.c`(2412727) |
| `kern/mm/pmm.c:90`(`//lab2 请补充`) | `page_init()`:基于 DTB 探测结果建立页描述符数组 | 本组 lab3 `pmm.c`(同段实现原样迁移) |
| `kern/trap/trap.c:86`(`//lab3 请补充`) | `interrupt_handler()`、`exception_handler()` | 本组 lab3 `trap.c`(lab3 function2 2412727) |
| `kern/trap/trapentry.S` `RESTORE_ALL`、`__alltraps` 尾部(`#lab3:填写`) | 上下文恢复宏与 trap 调用入口 | 本组 lab3 `trapentry.S`(lab3 function1 2411046) |

迁移中修复上游骨架的两处编译缺陷:

1. `kern/mm/kmalloc.c:92`:`kva2page(kva)` 传入 `uintptr_t` 未转型,GCC 15 默认按错误处理,补 `(void *)` 显式转换。
2. `kern/mm/memlayout.h`:上游骨架缺少 `PHYSICAL_MEMORY_END` 宏,补回 lab3 中已有的 `0x88000000`(与评分脚本 QEMU 128MiB 内存一致)。

**基线验证结果(Fedora,`riscv64-unknown-elf-gcc 15.1.0`):**

- `make clean && make`:25 个编译单元全部编译通过,无编译警告。
- 链接阶段按预期失败,未定义符号恰好为 4 个练习函数:`page_insert`、`page_remove`(功能模块一)、`proc_init`(功能模块二)、`cpu_idle`(功能模块三);三个功能模块实现后链接闭环。
- `make qemu`/`make grade` 待功能模块实现后执行。

**骨架附带测试设施说明:** `tools/test_lab4.c` 提供 `test_lab4_pmm_basic`、`test_lab4_process_create`、`test_lab4_schedule` 三个测试函数(对应 `make grade` 的 `[Test PMM]/[Test Process Create]/[Test Schedule]` 三项),但上游骨架中没有任何调用点,`tools/` 目录也不在内核构建路径内;测试挂接属于本实验实现内容,由各模块实现时一并完成。

【待完成:功能模块实现后的 `make qemu` 输出与 `make grade` 通过截图,引用 `./images/` 相对路径。】

---

## 六、实验总结与收获

### 对操作系统的理解

1. 【待完成:本实验重要知识点与 OS 原理对应关系,例如页表映射维护/引用计数/TLB 一致性、PCB 与线程生命周期、上下文切换的两级现场、调度原子性。】
2. 【待完成:OS 原理中重要但本实验未覆盖的知识点,例如页面置换、按需调页、用户态进程隔离、负载均衡与优先级调度等。】

### AI 协作开发的经验

【待完成】

### 成员 B 验证补充（2026-11-13）

[详细验证记录](report-memberB.md)。重新核查发现 `make grade` 已调用 `tools/inject.py`，会在临时副本挂接三个测试；上面的“没有任何调用点”仅适用于普通构建，不能据此重复添加永久挂接。B 增强了创建测试断言。完整 QEMU/grade 仍待 A/C 模块完成。
