# Lab3 Prompt 记录

本文件汇总 Lab3 实际使用的提示词和迭代过程。成员 A、B 完成各自模块后，把四段式 `[PROMPT]` / `[RELY]` / `[GUARANTEE]` / `[SPECIFICATION]` 提示词和问题记录追加到对应章节；成员 C 汇总并核对事实边界。

## Prompt A：Trap 上下文恢复与入口

~~~text
[PROMPT]
请阅读实验报告模板、lab3-task-split.md、lab3 实验要求和成员 A 已完成的代码说明，帮助我按照模板整理一份成员 A 的实验报告。报告应重点解释 TrapFrame 布局、SAVE_ALL 与 RESTORE_ALL 的对应关系、__alltraps 如何按 RISC-V 调用约定传递参数，以及测试计划和知识点。

[RELY]
- 只依据实验模板、lab3 任务分工和工程中的实际文件组织报告。
- 成员 A 负责 kern/trap/trapentry.S 的 RESTORE_ALL 和 __alltraps。
- 成员 B 负责 trap.c 的中断与异常处理，成员 C 负责集成。
- 图片位置使用 Markdown 图片占位符。

[GUARANTEE]
- 不生成或修改任何实验代码。
- 不虚构 make、QEMU、GDB 或 grade 的运行结果。
- 不把截图命令写进报告正文。
- 对尚未完成的成员 B/C 内容明确标记为待联调。
- 报告以文本解释为主，保留实验要求中的整体逻辑、核心函数、Spec、AI 协作边界、知识点和未覆盖知识点。

[SPECIFICATION]
输入：实验模板、lab3 分工、lab3 框架文件和成员 A 的代码范围。
输出：report/lab3-A.md，按模板组织章节，包含实验信息、整体执行流、TrapFrame 字段、汇编函数解释、Spec、实现风险、图片占位、OS 原理对应关系和总结。
约束：图片只留引用位置，截图命令另行提供；没有真实证据的测试只写预期和验证边界。
~~~

## Prompt B：中断与异常处理

~~~text
[PROMPT]
请按 Lab3 实验报告模板，为成员 B（2412727 刘蔚霖）撰写独立报告。
重点说明两个问题：内核如何持续接收时钟中断并按时关机？非法指令或断点触发异常后，内核如何从下一条指令继续执行？

[RELY]
- 分工：成员 A 提供 Trap 入口与恢复逻辑，成员 B 实现 interrupt_handler() 和 exception_handler()。
- 源码：结合 kern/trap/trap.c 和 kern/driver/clock.c 解释时钟处理、异常分发及返回地址更新。
- 证据：依据 tools/check_member_b.py 和实际运行日志核对异常地址、时钟输出次数及 QEMU 退出状态。

[GUARANTEE]
- 只整理报告。构建、评分和异常返回的结论均须有实际输出支持。
- 区分课程要求与实现选择，写明 SHUTDOWN_AFTER 默认取 10 的依据。
- 写明评分时使用的启动参数和临时页表适配。仅引用已有截图，待联调事项单独标明。

[SPECIFICATION]
输入：实验报告模板、课程练习、Lab3 分工表、成员 B 源码与验证记录。
输出：report/lab3-B.md，按模板组织以下内容。
1. 执行流：从 Trap 入口保存现场，跟踪到 C 层分发、处理以及 sret 返回。
2. 时钟：说明如何重设下一次事件、递增 ticks、每 100 个 tick 输出一次，并在第十次输出后关机。
3. 异常：说明如何读取首个半字判断指令长度，用四条 16/32 位非法指令和断点的实测 PC 及后续执行标志验证返回，并交代未知原因的处理方式。
4. 复验与总结：列出复现命令、关键输出、真实联调问题、spec/prompt 的设计依据，以及涉及和未覆盖的 OS 原理。
~~~

### 成员 B 开发提示词

以下按四段整理本次开发请求及核对源码后采用的 spec，非逐字对话。

~~~text
[PROMPT] 在既有 lab3 分支完成 B 的两个处理函数，使用 A 的返回路径联调，并提供可重放的时钟与异常测试。

[RELY] trap_dispatch 已按 scause 最高位区分同步与异步事件。A 从 TrapFrame 恢复 sepc。clock_init 将 ticks 清零并安排首个事件。实验运行在 RV64GC 内核态，故障指令地址可读。

[GUARANTEE] 保留函数签名、TrapFrame、分发器与 A 的汇编。每次时钟中断只增加一次 ticks，异步返回保留原 PC。非法指令和断点返回到下一条指令。未知原因打印完整现场。

[SPECIFICATION] 先重设时钟，再计数。每 TICK_NUM 次调用 print_ticks，达到 SHUTDOWN_AFTER 次统计时调用 sbi_shutdown。根据首半字低两位判断本实验的 2/4 字节指令。用评分脚本核对消息前缀，在隔离副本里连续执行两种宽度的非法指令和断点，用下一条指令的地址、后续标志和关机退出验证返回路径。
~~~

## Prompt C：框架导入、lab2 迁移与集成

### Prompt C-1：导入 lab3 框架并迁移 lab2

这是成员 C 在 M0 阶段实际执行的提示词整理版。目标是让 A/B 能在同一份 lab3 框架上并行开发，不实现 Trap 功能本身。

```text
[PROMPT]
**任务**：在 code/labcodes/lab3/lab3_new 导入 lab3 原始框架，并把 lab2 中已完成的物理内存管理代码迁移到 lab3 框架中标有“lab2”的位置。保留成员 A 的 kern/trap/trapentry.S 和成员 B 的 kern/trap/trap.c 中“lab3 function1/2 YOUR CODE”区域，不实现 Lab3 的 Trap 功能。

**操作要求**：直接修改工程中的实际文件；先读取 lab3 框架和 lab2 参考实现，逐项对照后再迁移；不覆盖 A/B 的占位；不引入与 Lab3 无关的功能；不重构 Makefile 和目录结构。

**输出要求**：完成框架导入、lab2 迁移、必要的工具链兼容修正和最小可编译脚手架；列出改动文件、构建命令、实际输出和未验证边界。不要虚构测试结果。

[RELY]
可读取的 lab3 框架文件：
- code/labcodes/lab3/lab3_new/kern/mm/pmm.c
- code/labcodes/lab3/lab3_new/kern/mm/pmm.h
- code/labcodes/lab3/lab3_new/kern/mm/memlayout.h
- code/labcodes/lab3/lab3_new/kern/mm/default_pmm.c
- code/labcodes/lab3/lab3_new/kern/mm/best_fit_pmm.c
- code/labcodes/lab3/lab3_new/kern/trap/trapentry.S
- code/labcodes/lab3/lab3_new/kern/trap/trap.c
- code/labcodes/lab3/lab3_new/kern/trap/trap.h
- code/labcodes/lab3/lab3_new/kern/sync/sync.h
- code/labcodes/lab3/lab3_new/Makefile

lab2 参考实现：
- code/labcodes/lab2/lab2_new/kern/mm/pmm.c
- code/labcodes/lab2/lab2_new/kern/mm/best_fit_pmm.c
- code/labcodes/lab2/lab2_new/kern/mm/default_pmm.c
- code/labcodes/lab2/lab2_new/kern/mm/memlayout.h

真实接口与状态：
- page_init()、init_memmap()、alloc_pages()、free_pages()、nr_free_pages()
- struct pmm_manager、best_fit_pmm_manager、default_pmm_manager
- PADDR()、ROUNDUP()、ROUNDDOWN()、pa2page()、page2pa()
- npage、nbase、pages、va_pa_offset
- PHYSICAL_MEMORY_OFFSET、PHYSICAL_MEMORY_END、DRAM_BASE、PGSIZE
- PageReserved()、PageProperty()、SetPageProperty()、ClearPageProperty()
- local_intr_save()、local_intr_restore()（lab3 的 pmm.c 包装层使用）
- struct trapframe、trap()、interrupt_handler()、exception_handler()
- SAVE_ALL、RESTORE_ALL、__alltraps、__trapret、stvec、sscratch、stval、sret

工具链与运行条件：
- 交叉编译器前缀：riscv64-unknown-elf-
- 模拟器：QEMU 4.1.1
- QEMU 启动参数需使用 -device loader,file=bin/ucore.img,addr=0x80200000

[GUARANTEE]
必须完成或修改的接口/文件：
- kern/mm/pmm.c：补回 page_init()；默认选择 best_fit_pmm_manager；保留 USE_FIRST_FIT_PMM 作为对照；保留 lab3 的中断保护包装。
- kern/mm/best_fit_pmm.c：合入 lab2 的 Best-Fit 初始化、选块、切分、释放合并和 best_fit_pmm_manager 注册。
- kern/mm/default_pmm.c：补齐尾部插入后的 break，避免链表遍历继续。
- kern/mm/memlayout.h：补充 PHYSICAL_MEMORY_END。
- kern/trap/trapentry.S：只做工具链兼容修正，把旧 CSR 名 sbadaddr 改为 stval；RESTORE_ALL 和 __alltraps 的实现保持空白。
- kern/trap/trap.h：补充 interrupt_handler() 和 exception_handler() 声明。
- kern/trap/trap_stub.c：提供弱符号占位，保证 A/B 实现前框架可链接；不得替成员 B 实现真实中断/异常处理逻辑。

允许新增：
- 必要的 static 辅助函数和局部宏；
- trap_stub.c 这样的最小脚手架文件。

禁止：
- 修改成员 A 的 RESTORE_ALL / __alltraps 实现区域；
- 修改成员 B 的 interrupt_handler / exception_handler 实现区域；
- 引入 Buddy System、SLUB 或其他与 lab3 框架无关的模块；
- 修改 Makefile 的编译选项来绕过错误；
- 把未执行的构建或 QEMU 结果写成通过。

[SPECIFICATION]
## M0：lab3 框架导入与 lab2 迁移

**Pre-Condition**：
- lab3 原始框架已经放入 code/labcodes/lab3/lab3_new。
- lab2 分支中存在可参考的 page_init() 和 Best-Fit 实现。
- A/B 的 YOUR CODE 占位保持原样。
- 当前工具链支持 RISC-V 64，且 QEMU 为 4.1.1。

**Post-Condition**：
- pmm.c 中存在 page_init()，能根据 DTB 的 memory base/size 设置 va_pa_offset、npage、pages，排除内核与描述符数组，并把完整空闲页交给 init_memmap()。
- 默认页级管理器为 best_fit_pmm_manager；USE_FIRST_FIT_PMM 可切换回 First-Fit。
- lab3 框架能从 clean 状态完成 make GCCPREFIX=riscv64-unknown-elf-，生成 bin/kernel 和 bin/ucore.img。
- A/B 的 YOUR CODE 未被填充；trap_stub.c 只提供弱符号占位。
- 报告只记录实际构建和 QEMU 启动到时钟初始化的结果，不把 Trap 功能写成已通过。

**Case 1：缺少 PHYSICAL_MEMORY_END**
- 触发：lab3 memlayout.h 没有该宏，而 page_init() 需要限制物理内存上界。
- 处理：补充与框架一致的 PHYSICAL_MEMORY_END，不改变原有 PHYSICAL_MEMORY_OFFSET。

**Case 2：旧 CSR 名导致汇编失败**
- 触发：trapentry.S 使用 sbadaddr，当前工具链报 unknown CSR。
- 处理：只把 SAVE_ALL 中的 sbadaddr 改为 stval，不修改 SAVE_ALL 的字段布局和 A 的恢复逻辑。

**Case 3：A/B 未实现导致链接失败**
- 触发：trap_dispatch 调用 interrupt_handler()/exception_handler()，但 trap.c 还没有定义。
- 处理：在 trap.h 增加声明，在 trap_stub.c 提供 weak 占位；成员 B 的强符号实现后自动覆盖，不产生重复定义。

**Case 4：构建或运行失败**
- 触发：编译、链接、objcopy 或 QEMU 启动失败。
- 处理：保留实际错误输出，记录为未完成或环境限制，不修改结论为通过。

**Requirements**：
- 只做 M0 需要的迁移和脚手架，不提前实现 Lab3 Trap 功能。
- 所有路径使用 OS2026 内相对路径；不写入主机绝对路径、凭据或内部笔记。
- 构建命令、关键输出和未验证边界必须写入报告。

### Prompt C-2：A/B 合入后的运行时联调验证

这是成员 C 在 A/B 实现合入后实际执行的联调提示词整理版。目标是把两个功能模块的静态实现落实为可观察的运行时证据，并对遗留决策给出有输出支撑的结论。

```text
[PROMPT]
**任务**：在成员 A/B 的实现合入后，对 lab3 仓库默认形态做运行时联调验证，不修改任何实验功能代码。完成三件事：① 用 make debug 启动暂停的 QEMU，用 riscv64-unknown-elf-gdb 在 __alltraps、trap()、__trapret 设断点，验证 Trap 入口参数传递、TrapFrame 布局和恢复返回闭环；② 以仓库默认启动参数运行 make qemu 和 make grade，记录实际输出；③ 对成员 B 交接的两个遗留事项给出结论——默认启动方式是否存在其报告的"OpenSBI 下一跳为零"问题，以及 SHUTDOWN_AFTER 默认取 10 与课程评分是否兼容。

**操作要求**：命令在 code/labcodes/lab3/lab3_new 中执行；GDB 验证用批量脚本记录完整会话；make grade 结束后重建普通内核再确认工作区状态；发现代码问题只报告并定位到负责人模块，不擅自修改功能实现。

**输出要求**：列出实际执行的命令、关键输出和逐项结论；GDB 每个断点处的寄存器与内存值如实记录；不虚构任何测试结果。

[RELY]
- 分工与实现状态：A 的 RESTORE_ALL/__alltraps 已合入（kern/trap/trapentry.S，标记 lab3 function1 2411046）；B 的 interrupt_handler/exception_handler 已合入（kern/trap/trap.c，标记 lab3 function2 2412727，SHUTDOWN_AFTER 未定义时默认 10，可编译宏覆盖）。
- 真实接口与状态：__alltraps、__trapret、trap(struct trapframe *)；struct trapframe 为 36 项布局（32 个 GPR + sstatus/sepc/stval/scause，REGBYTES=8）；idt_init() 将 stvec 指向 __alltraps；时钟中断的 scause 为中断位（最高位 1）加 IRQ_S_TIMER；clock_set_next_event()、ticks、TICK_NUM=100、sbi_shutdown()。
- Makefile 实际目标：qemu 为 -machine virt -nographic -bios default -device loader,file=bin/ucore.img,addr=0x80200000；debug 附加 -s -S；gdb 以 riscv64-unknown-elf-gdb 连接 localhost:1234；工具链前缀 riscv64-unknown-elf-，模拟器为课程要求的 QEMU 4.1.1（成员 C 本机分别位于 /opt/riscv/bin 与 /opt/qemu-4.1.1/bin）。
- 待确认事实：成员 B 报告其本机 QEMU 11.1.2 下原始 make qemu 的 OpenSBI 下一跳为零，需临时页表恒等映射和 -kernel 参数才能启动；课程要求环境为 QEMU 4.1.1。
- make grade 依赖注入机制（输出 lab3 injection complete），依次运行上下文恢复与断点 EPC、非法指令 EPC 前进与返回、持续时钟与重复 Trap 返回三项测试，输出各项分数与 Total Score。

[GUARANTEE]
不修改 kern/ 下任何实验功能代码；只允许临时 GDB 批量脚本，会话结束后不留在仓库。
必须完成的检查项：
- GDB：__alltraps 处 scause 为时钟中断值（最高位 1 + IRQ_S_TIMER）；trap() 入口 a0 == sp 且指向 trapframe；trapframe 内保存的原始 sp、sepc、scause 与 CSR 寄存器一致；单步跨过 RESTORE_ALL 后 sp 恢复为保存值、sepc/sstatus 写回对应 CSR；执行 sret 后 pc 等于保存的 tf->epc。
- make qemu：默认参数下输出十次 100 ticks 后经 sbi_shutdown 正常退出（QEMU 退出码 0）。
- make grade：三项测试全部 PASS（35/35 + 25/25 + 40/40），Total Score 100/100。
- 结论：默认启动方式与 SHUTDOWN_AFTER=10 的兼容性各给出一条有命令输出支撑的明确结论。
禁止：把未执行的检查写成通过；修改 Makefile 或启动参数来绕过启动问题；把注入测试后的构建产物留作最终交付状态。

[SPECIFICATION]
## C-M2：运行时联调验证与遗留事项确认

**Pre-Condition**：
- A/B 实现已合入，make clean && make GCCPREFIX=riscv64-unknown-elf- 可从干净状态完成构建。
- 本机可提供 riscv64-unknown-elf 前缀工具链与 QEMU 4.1.1。

**Post-Condition**：
- GDB 会话逐项确认模块一闭环：参数传递、TrapFrame 布局、恢复顺序与 sret 返回地址，全部有会话记录。
- make qemu 在仓库默认形态完整跑通；make grade 三项 100/100。
- 成员 B 交接的两个事项得到结论：课程要求的 QEMU 4.1.1 下默认启动无需任何适配（其问题为 QEMU 11 行为差异）；SHUTDOWN_AFTER=10 与默认 make qemu 和 make grade 均兼容。
- 工作区无注入残留，最终交付状态为普通内核构建。

**Case 1：GDB 断点未命中 __alltraps**
- 触发：QEMU -S 暂停状态下设断点后 continue 不命中。
- 处理：核对 stvec 写入与内核加载地址 0x80200000，记录实际现象；属环境问题时记录环境限制，不修改内核代码。

**Case 2：默认 make qemu 启动失败或输出异常**
- 触发：默认参数下 OpenSBI 不跳转或内核提前退出。
- 处理：保留完整输出，区分 QEMU 版本差异与内核问题；课程要求版本下仍失败的记录为未解决，不写成通过。

**Case 3：make grade 分数与预期不符**
- 触发：某项 FAIL 或总分低于 100。
- 处理：保留评分输出与失败消息，定位到 A/B 对应处理函数并交由负责人修复；联调方不直接改动功能代码。
```

```
