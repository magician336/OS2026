# 成员 B：刘蔚霖（2412727）实现与验证

日期：2026-11-13。依据课程[练习](http://oslab.mobisys.top/lab2026/_book/lab4/lab4_2_1_exercises.html)及实际头文件、entry.S、trapentry.S；起点为 lab4 的 2f1b83b。工具实测：macOS，GCC 16.2.0、GDB 18.1、QEMU 11.1.2。使用 Codex，底层精确模型版本未核实，不填写猜测值。首次交付时 prompt.md 按要求留空；现已补充[成员 B 提示词与实际协作说明](prompt.md#member-b-prompt)。新增提示词用于后续手写练习，与本次由 Codex 实现核心代码的实际过程分别记录。

## 验收清单与接口

| 项目 | 实现与证据 | 范围 |
| --- | --- | --- |
| alloc_proc | 整块清零，pid=-1，PROC_UNINIT，pgdir=boot_pgdir_pa，链表节点自环；注入 kmalloc 失败返回 NULL | 宿主实测 |
| proc_init | 初始化全局链表和哈希桶；idle PID 0，启动栈、need_resched=1、current、计数 1；创建并查找 init PID 1，设置名称，失败 panic | 正常路径宿主实测，panic 路径代码检查 |
| kernel_thread | s0 函数、s1 参数；SPP/SPIE=1、SIE=0；epc=kernel_thread_entry | 字段实测与汇编核对 |
| copy_thread | 288 字节陷阱帧位于 8192 字节栈顶；16 字节对齐；a0=0；默认 sp 指向陷阱帧；显式 stack 被保留 | 宿主实测 |
| do_fork | 数量上限、PCB/栈分配、父子关系、发布顺序、PID 与计数 | 成功及两种分配失败实测 |
| copy_mm | 断言 current->mm==NULL，子 mm=NULL；不复制用户空间 | 正常路径实测；约束代码检查 |
| 首次执行 | context.ra=forkret、context.sp=tf；后续由 C 的切换恢复 | 字段验证通过，真实 CPU 执行待集成 |

idle 不进入普通进程链表或 PID 哈希，find_proc 只接受正 PID。它计入 nr_process；调度器无普通可运行线程时以 idle 保底。所有线程共享内核页表，pgdir 保存物理地址而非 satp 编码。C 切换必须使用 Sv39 编码，不可调用 Sv32 遗留 lsatp。

## 创建到运行

proc_init → kernel_thread → do_fork → alloc_proc/setup_kstack/copy_mm/copy_thread → 分配 PID、挂接、计数、wakeup_proc。初始化期间 current 始终为 idle。

C 完成调度后，switch_to 恢复 context.ra/sp，进入 forkret；forkrets 将 sp 指向 current->tf；__trapret 恢复寄存器和 CSR（控制与状态寄存器），sret 根据 SPP 进入 S 模式，根据 SPIE 恢复中断；epc 指向 kernel_thread_entry。该入口将 s1 搬到 a0，并 jalr s0 执行 init_main。函数返回后进入 do_exit，当前骨架的 do_exit 会 panic，这不是完整退出回收实现。

context 只保存调用约定要求的返回地址、栈和被调用者保存寄存器；trapframe 保存完整异常返回现场。这是两种不同用途的保存层次。当前实验未实现用户地址空间复制、用户隔离、wait/exit 完整生命周期或多核并发创建。

## 回收与迭代

达到 MAX_PROCESS 返回 -E_NO_FREE_PROC；PCB 分配失败返回 -E_NO_MEM；栈分配失败释放 PCB；copy_mm 将来若返回错误，先释放栈再释放 PCB。当前 copy_mm 只有成功或断言失败，因此其返回错误分支为后续扩展保留，未声称已注入测试。所有可失败步骤在挂接前完成，避免失败留下半个进程。发布阶段关闭中断，先挂接再唤醒。本实验单核、无创建过程抢占；这不是多核锁实现。

set_proc_name 原接口固定复制 15 字节，故 idle/init 名称使用足够长的静态数组，避免从短字符串字面量越界读取。没有修改他人的调度函数。测试注入暴露的原测试线程 void/int 签名问题只做接口修复，没有改变调度测试行为。

## 实测及复现

在 `code/labcodes/lab4/lab4_new` 运行：

```sh
python3 tools/check_member_b.py
make GCCPREFIX=riscv64-elf-
```

第一条输出 `member B: PCB, init, publication, trapframe, PID and allocation rollback PASS`。脚本读取实际 proc.c、proc.h、trap.h、list.h，在临时目录编译运行；只替换内存分配、CSR、中断和汇编入口等外部依赖。分配器先填 0xa5，防止零内存掩盖初始化缺失；检查失败前后资源计数和链表不变，唤醒替身验证线程已经登记。它没有执行 RISC-V 指令，不证明页表或调度正确。

第二条本次进程编译通过，但最终链接仍失败：缺少 A 的 page_insert/page_remove 和 C 的 cpu_idle。基线还缺 proc_init，实施后该未定义符号已消失。因此未获得整机 QEMU 输出或 grade 分数，不提供伪造截图。本次证据为命令及文本记录，没有真实终端截图。

评分路径实际已存在：make grade 复制目录到临时目录，tools/inject.py 注入 PMM、创建和调度测试。已验证注入成功，并增强 test_lab4_process_create 的页表、栈位置、对齐、返回值和状态位断言。待 A/C 集成后运行 `make GCCPREFIX=riscv64-elf- qemu` 和 `make GCCPREFIX=riscv64-elf- grade`；创建项应输出 `[Test Process Create] PASS`，实际分数由完整运行确定。现代 QEMU 启动兼容性本次因链接阻塞尚未验证，未搬入旧实验补丁。

注入副本已实际完成编译阶段；链接除普通构建缺项外还缺 C 的 schedule。测试回调补不可达 return 0 以满足编译器返回值检查。
