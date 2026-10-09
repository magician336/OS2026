# Lab1 Prompt 汇总

## 小组信息

| 成员 | 学号 | 负责范围 |
|---|---:|---|
| 梁家瑞 | 2411046 | 工程结构、构建流程和 QEMU 运行 |
| 刘蔚霖 | 2412727 | 启动流程、练习 1/2 和 GDB 验证 |
| 李培涛 | 2411041 | SBI/console、报告整合和交付检查 |

本文件按成员和任务整理本实验实际使用的提示词。这些提示词用于辅助完成成员 A、B、C 的代码阅读、调试、报告撰写和交付整理；为便于教师审阅，保留任务意图并统一为 `[PROMPT]`、`[RELY]`、`[GUARANTEE]`、`[SPECIFICATION]` 四段式。

---

## 成员 A：代码结构与构建流程

### Prompt A-1：阅读代码结构和构建流程

```text
[PROMPT]
我负责 OS2026 Lab1 的工程结构、构建流程和 QEMU 运行部分。请阅读 code/labcodes/lab1 中的 Makefile、tools/kernel.ld、kern/init/entry.S 和 kern/init/init.c，整理一份能够解释源码、构建、链接和启动关系的分析。

[RELY]
- 以 code/labcodes/lab1 中的当前源码和 Makefile 为准。
- 重点关注 kern_entry、kern_init、BASE_ADDRESS、bin/kernel 和 bin/ucore.img。

[GUARANTEE]
- 不修改 Lab1 源码，不引入无关功能。
- 区分源码确认、命令实测和环境限制。
- 未执行的 QEMU 或测试命令不得写成成功。

[SPECIFICATION]
- 说明交叉编译、链接脚本、objcopy 和 QEMU 的关系。
- 说明入口地址、栈设置、C 语言初始化和启动输出之间的顺序。
- 给出实际验证命令、关键输出和报告截图建议。
```

## AI 使用说明

本次使用 AI 的范围仅限于辅助阅读 Lab1 代码和理解构建流程，包括 Makefile 如何组织编译、链接脚本如何指定入口和地址、entry.S 如何进入 kern_init，以及 QEMU 如何加载并运行内核镜像。

---

## 成员 B：启动流程与 GDB 验证

以下三条提示词实际用于成员 B 的代码分析、GDB 验证和报告撰写，现按报告提交格式统一整理。实验结论必须以实际源码、命令输出和 GDB 截图为准。

### Prompt B-1：分析内核入口代码

```text
[PROMPT]
请阅读 OS2026/code/labcodes/lab1/kern/init/entry.S、tools/kernel.ld、kern/init/init.c 和 kern/mm/memlayout.h，解释内核入口、启动栈和从汇编进入 C 函数的过程。

[RELY]
- kernel.ld 使用 ENTRY(kern_entry)，并将内核基地址设置为 0x80200000。
- entry.S 中包含 la sp, bootstacktop 和 tail kern_init。
- KSTACKSIZE 由 KSTACKPAGE 和 PGSIZE 决定；栈从高地址向低地址增长。

[GUARANTEE]
- 只依据当前源码解释指令和地址，不使用未核对的旧版本结论。
- 不修改 entry.S、kernel.ld 或初始化代码。
- 区分源码结论和需要 GDB 实测的寄存器值。

[SPECIFICATION]
- 解释 kern_entry、la sp、bootstacktop、tail kern_init 和 kern_init 的关系。
- 说明每个结论应使用的源码位置、反汇编或寄存器命令。
- 对无法从源码确认的具体地址标记为待验证。
```

### Prompt B-2：规划 GDB 启动流程验证

```text
[PROMPT]
请为 OS2026 Lab1 设计一套可复现的 QEMU/GDB 启动流程验证，覆盖复位地址、OpenSBI、kern_entry、内核栈和 kern_init，并给出适合放入报告的截图步骤。

[RELY]
- 正式验证优先使用 QEMU 4.1.1 和项目指定的 RISC-V 工具链。
- QEMU 复位 ROM、OpenSBI 固件和 Lab1 内核是不同层次，不能混为同一份源码。
- Makefile 提供 make debug 和 make gdb；GDB 远程端口为 localhost:1234。

[GUARANTEE]
- 只记录实际执行过的命令、地址、寄存器和输出。
- 如果使用其他 QEMU 或工具链版本，必须标记为补充环境，并说明版本差异。
- 不把单张 QEMU 输出图当作完整调用链或寄存器证据。

[SPECIFICATION]
- 记录连接暂停 QEMU、检查 pc/sp、x/6i $pc、si、断点 0x80200000 和读取 bootstacktop 的命令。
- 验证 0x1000 复位 ROM、OpenSBI 入口和 0x80200000 内核入口之间的控制权转移。
- 每张截图说明命令、可观察结果、文件名和报告结论；截图必须来自实际运行。
```

### Prompt B-3：整理成员 B 独立报告

```text
[PROMPT]
请依据成员 B 实际的源码阅读、QEMU/GDB 输出和截图，撰写 Lab1 成员 B 独立报告，回答练习 1 和练习 2，并为最终总报告提供可复核材料。

[RELY]
- 报告范围是 entry.S、kernel.ld、kern_init、QEMU 复位 ROM、OpenSBI 和 GDB 验证。
- 需要解释 la sp、tail kern_init、0x1000、0x80000000、0x80200000 和控制权转移。
- 报告图片只能引用 OS2026/report/images/ 中实际存在的文件。

[GUARANTEE]
- 每个地址、寄存器值和测试结论都要标明源码、命令或截图来源。
- 明确区分正式 QEMU 4.1.1 验证、其他版本补充验证和未验证内容。
- 不覆盖总报告，不伪造姓名、模型、测试结果或评分结果。

[SPECIFICATION]
- 按实验模板组织实验环境、练习答案、实现过程、验证证据和限制。
- 将提示词、命令、截图文件名和结论建立一一对应关系。
- 最终输出可以合并到 OS2026/report/report.md，并保留待补信息标记。
```

---

## 成员 C：SBI/console、截图与交付

以下提示词实际用于成员 C 的 SBI/console 理解、QEMU 环境记录、截图整理、草稿交付和答辩学习，并按报告提交格式整理。

### Prompt C-1：修正 WSL 中的 `use-qemu`

```text
[PROMPT]
我在 WSL Ubuntu 的 zsh 环境中完成 OS2026 Lab1。请检查并修正当前用户配置中的 use-qemu 函数，使它能够稳定选择 QEMU 4.1.1，并保留已有的 QEMU 7.0.0 选择能力。直接修改实际配置文件，不要只给出示例代码。修复后验证新 shell 和当前 shell 的行为，并说明任何环境限制。

[RELY]
- Lab1 在 WSL 中通过 use-qemu 选择 QEMU 版本。
- 当前函数使用 zsh 的 path 数组，并在函数内部去重 PATH。
- 需要使用 QEMU 4.1.1 执行 Lab1 的 make qemu 和 make debug。

[GUARANTEE]
- 保留函数名 use-qemu 及已有版本参数 use-qemu 4.1.1、use-qemu 7.0.0。
- 不改变两个 QEMU 安装目录中的可执行文件。
- 函数选择成功后应能在函数内和函数返回后通过 command -v qemu-system-riscv64 找到目标版本。

[SPECIFICATION]
- 输入 4.1.1：检查目标可执行文件存在且版本匹配，将目标目录置于 PATH 首位并刷新命令缓存。
- 输入 7.0.0：执行同样的检查和切换。
- 无参数：显示当前可用 QEMU；未选择时给出明确提示。
- 未知版本：返回非零状态并显示用法。
- 验证 use-qemu 4.1.1、use-qemu 7.0.0、重复切换和错误版本参数。
- 保留原配置备份，不修改 Lab1 仓库代码。
```

**事实来源：** WSL 中 use-qemu 的实际行为和 Lab1 的 QEMU 版本输出。
**验证映射：** 两个版本的 `--version`、`command -v`、重复切换和错误参数测试。

### Prompt C-2：固定 Lab1 的 QEMU 4.1.1 构建环境

```text
[PROMPT]
请为 Lab1 的构建、运行与验证记录明确写出：进入 WSL 后先执行 use-qemu 4.1.1，再运行 make qemu、make debug 或其他依赖 QEMU 的命令。保留现有实验目录、分支、报告和 Git 约束，不要改写无关内容。

[RELY]
- OS2026 是需要提交的 Git 仓库，实验分支命名为 labx。
- Lab1 使用 riscv64-unknown-elf 工具链和 qemu-system-riscv64。
- WSL 中已配置 use-qemu 4.1.1。

[GUARANTEE]
- 只修改实验环境和工作流记录，不修改 Lab1 源码。
- 保留现有 make、QEMU、报告截图和 Git 提交要求。
- 文档必须要求记录实际命令、输出和环境限制。

[SPECIFICATION]
- 在构建与验证章节写出 use-qemu 4.1.1 的执行顺序。
- 明确 QEMU 4.1.1 是本实验默认版本。
- 修改后检查文档内容，不产生 OS2026 代码或报告文件的额外修改。
```

**事实来源：** WSL QEMU 配置、OS2026 Makefile 和实验交付要求。
**验证映射：** 检查文档中的 use-qemu 4.1.1 文本，并确认 OS2026 工作区无额外修改。

### Prompt C-3：整理成员 C 的 SBI/console 报告草稿

```text
[PROMPT]
我负责 OS2026 Lab1 的成员 C 部分。请阅读以下实际文件，整理一份独立的中文报告草稿，不修改成员 A、B 的代码和总报告：
- OS2026/code/labcodes/lab1/libs/sbi.c
- OS2026/code/labcodes/lab1/libs/sbi.h
- OS2026/code/labcodes/lab1/kern/driver/console.c
- OS2026/code/labcodes/lab1/kern/libs/stdio.c
- OS2026/code/labcodes/lab1/libs/printfmt.c

[RELY]
- 以当前源码为准，不能凭记忆虚构函数签名、寄存器或服务号。
- 需要解释 cprintf -> vcprintf -> vprintfmt -> cputch -> cons_putc -> sbi_console_putchar -> sbi_call -> ecall -> OpenSBI 的链路。
- sbi_call 通过 x17、x10、x11、x12 传递服务号和参数。
- 报告应区分 QEMU 虚拟硬件、OpenSBI 固件和内核。

[GUARANTEE]
- 输出一份可独立保存的 Markdown 草稿。
- 记录每个文件和关键函数的职责、接口边界、关键不变量和验证限制。
- 不把未运行的 make、QEMU、GDB 或 grade 结果写成已通过。
- 未知姓名、学号、日期和模型版本保留待填写标记。

[SPECIFICATION]
- 给出完整调用链和每一层的输入、输出与职责。
- 解释 vprintfmt 的 putch 回调和 putdat 上下文设计。
- 解释 SBI 服务号 1、a7/x17、a0/x10 和 ecall。
- 说明 make qemu、make debug、make gdb 的验证命令和截图要求。
- 草稿最终应能整合到 OS2026/report/report.md，但本次先单独保存。
```

**事实来源：** 五个 Lab1 源文件、Makefile、课程实验手册和项目报告模板。
**验证映射：** 源码行号核对、调用链截图、QEMU 输出截图和 GDB 调用栈/反汇编截图。

### Prompt C-4：编写可执行的截图操作指南

```text
[PROMPT]
请为 Lab1 成员 C 编写具体截图操作指南。每张截图都要给出 WSL 命令、终端窗口要求、截图内容、建议文件名和报告引用方式。使用 QEMU 4.1.1，并覆盖版本检查、编译成功、make qemu 输出、cons_putc GDB 调用栈和 ecall 反汇编。

[RELY]
- Lab1 代码目录是 `OS2026/code/labcodes/lab1`。
- Makefile 提供 make、make qemu、make debug 和 make gdb。
- make debug 在 localhost:1234 等待 GDB；make gdb 连接 bin/kernel。
- 报告图片必须放入 OS2026/report/images/，使用相对链接。

[GUARANTEE]
- 每张建议截图必须对应真实命令和可观察输出。
- 不把缺少 tools/grade.sh 的 make grade 写成成功。
- 指出 QEMU 的退出方式和双终端 GDB 操作。

[SPECIFICATION]
- 版本图：use-qemu 4.1.1、QEMU 版本和 RISC-V GCC 路径。
- 编译图：make clean、make，以及 kernel 和 ucore.img 生成结果。
- 运行图：make qemu 和 `(THU.CST) os is loading ...`。
- 调用栈图：break cons_putc、continue、bt 6、info registers a0。
- ecall 图：disassemble /m sbi_call、x/i $pc、info registers a0 a7；没有寄存器证据时不得过度声称。
```

**事实来源：** Lab1 Makefile、实际 WSL 命令和当前截图。
**验证映射：** 每张图片的命令输出、截图文件存在性和报告相对链接。

### Prompt C-5：审查、重命名并归档截图

```text
[PROMPT]
请检查现有 Lab1 截图，判断哪些能用于成员 C 报告。用用途命名图片并复制到 OS2026/report/images/。只删除被最新截图明确替代的旧图，保留仍能证明 QEMU 版本、编译、运行和 ecall 的图片。不要覆盖 report/report.md。

[RELY]
- 最新 GDB 图应能清楚展示 cons_putc -> cputch -> vprintfmt -> vcprintf -> cprintf -> kern_init。
- QEMU 运行图应包含 OpenSBI 和 `(THU.CST) os is loading ...`。
- ecall 图用于证明反汇编中的 ecall；它不能自动证明寄存器值。
- 报告图片目录是 OS2026/report/images/。

[GUARANTEE]
- 使用可读的 member-c-*.jpg 文件名。
- 复制后源文件和目标文件内容一致。
- 报告草稿中的图片链接与实际文件名一致。
- 清理构建产物，不把 bin/、obj/ 提交到 Git。

[SPECIFICATION]
- 保留 member-c-qemu-version.jpg、member-c-build-success.jpg、member-c-make-qemu.jpg、member-c-ecall-disassembly.jpg、member-c-gdb-cons-putc.jpg。
- 只删除已被最新 GDB 图替代的旧 GDB 截图。
- 用文件哈希检查复制结果，并用 git status、git diff --check 检查交付状态。
```

**事实来源：** 现有图片、截图视觉检查、OS2026/report/images/ 和 Git 状态。
**验证映射：** 文件哈希一致、报告链接可定位、构建产物未进入暂存区。

### Prompt C-6：提交成员 C 草稿和图片到 GitHub

```text
[PROMPT]
请把成员 C 的报告草稿和五张图片提交到 OS2026 的 lab1 分支并推送到 origin/lab1。草稿作为成员 C 的独立材料提交，图片放在 report/images/；不要把草稿覆盖到 report/report.md，也不要提交构建产物。

[RELY]
- Git 仓库是 `OS2026`。
- 当前分支是 lab1，远程名是 origin。
- report/report.md 是总报告模板，report/prompt.md 是提示词汇总。
- 远程分支可能领先本地；推送被拒绝时先 fetch 并安全 rebase，禁止强制推送。

[GUARANTEE]
- 提交只包含成员 C 草稿和五张报告图片。
- 保留 report/report.md 原模板和已有成员提交。
- 提交前执行 git diff --check、git status 和报告图片链接检查。
- 使用带 lab1 范围的祈使式提交信息。

[SPECIFICATION]
- 如果远程领先，先 git fetch origin lab1，再将本地提交 rebase 到远程最新提交。
- 发生冲突时停止并报告冲突文件，不覆盖他人内容。
- 推送完成后确认 HEAD 与 origin/lab1 一致且工作区干净。
```

**事实来源：** OS2026 Git 状态、远程地址、成员 C 草稿和图片目录。
**验证映射：** 提交文件清单、非强制 push 输出、最终远程 SHA 和干净工作区。

### Prompt C-7：以答辩模式教授成员 C 的 Lab1

```text
[PROMPT]
请作为操作系统 Lab1 答辩教练，带我掌握成员 C 的 SBI/console 部分。先用短课解释真实源码，再一次只问一个问题进行主动回忆；根据我的回答纠正术语，记录已经掌握的概念，并继续追问。不要让我死记报告。

[RELY]
- 重点源码是 sbi.c、sbi.h、console.c、stdio.c 和 printfmt.c。
- 核心链路是 cprintf -> vcprintf -> vprintfmt -> cputch -> cons_putc -> sbi_console_putchar -> sbi_call -> ecall -> OpenSBI -> QEMU console。
- sbi_call 使用 x17/a7 传服务号，x10/a0 传字符，x11/a1 和 x12/a2 在 console putchar 中为 0。
- Makefile 使用 -O2 -g，因此 GDB 可能显示 optimized out；截图证据必须区分编译、调用栈、反汇编和运行输出。

[GUARANTEE]
- 每次只提出一个答辩问题，并在回答后指出正确点、遗漏点和术语修正。
- 把“QEMU 模拟虚拟硬件”“OpenSBI 提供固件服务”“ecall 触发特权级陷入”区分清楚。
- 不把未观察到的寄存器值、测试结果或函数调用写成事实。

[SPECIFICATION]
- 先解释函数指针 putch、上下文指针 putdat 和 va_list ap。
- 再练习 cprintf、SBI 参数和 QEMU/OpenSBI 分层。
- 最后用截图证据和 30 秒完整口述进行模拟答辩。
- 对错误答案给出更准确的可背诵表述，并要求用户重新回答关键句。
```

**事实来源：** 实际 Lab1 源码、Makefile、成员 C 报告草稿、五张截图和用户在本会话中的回答。
**验证映射：** 函数链口述、a7/a0 寄存器问答、GDB `bt 6`、`ecall` 反汇编和 QEMU 输出截图。

### 统一迭代记录

- 初始任务是修复 WSL 中 `use-qemu`，后续补充了项目文档中的 QEMU 4.1.1 约束。
- 成员 C 明确要求只处理 SBI/console 理解、报告草稿和截图交付；草稿作为独立材料整理，不覆盖 `report/report.md`。
- 截图整理过程中先审查可用性，再用有意义的文件名复制到 `report/images/`，并清理 `bin/`、`obj/` 构建产物。
- GitHub 推送遇到远程领先时，先 fetch/rebase，再推送，禁止 force push。
- 答辩教学从解释 `vprintfmt` 开始，逐步覆盖回调、`putdat`、SBI 寄存器、OpenSBI/QEMU 分层和证据边界。
