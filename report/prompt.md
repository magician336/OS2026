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

[待成员 B 填写：最终提示词、迭代过程、时钟/异常测试中遇到的问题]

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
```