# Lab1 成员 C：SBI 与 console 输出链路

> 本文档是成员 C 的独立报告草稿，最终整合到 `OS2026/report/report.md`。
> 当前仓库中的 Lab1 实际路径为 `OS2026/code/labcodes/lab1`。

## 1. 负责范围

成员 C 负责理解 SBI 与 console 输出模块，并整理报告、提示词、截图和 Git 交付结构。本文重点分析：

- `OS2026/code/labcodes/lab1/libs/sbi.c`
- `OS2026/code/labcodes/lab1/libs/sbi.h`
- `OS2026/code/labcodes/lab1/kern/driver/console.c`
- `OS2026/code/labcodes/lab1/kern/libs/stdio.c`
- `OS2026/code/labcodes/lab1/libs/printfmt.c`

## 2. 输出链路总览

一次内核格式化输出的主要调用链是：

```text
cprintf
  -> vcprintf
    -> vprintfmt
      -> cputch
        -> cons_putc
          -> sbi_console_putchar
            -> sbi_call
              -> ecall
                -> OpenSBI
                  -> 控制台输出
```

`vprintfmt` 不直接依赖具体设备。它接收一个 `putch` 回调；内核标准输出场景把 `cputch` 作为回调传入，因此格式化逻辑与底层 SBI 设备接口保持分离。

## 3. 各模块职责

### 3.1 `cprintf` 与 `vcprintf`

`kern/libs/stdio.c:25-29` 的 `vcprintf` 初始化字符计数器 `cnt`，把 `cputch` 和 `&cnt` 传给 `vprintfmt`，最后返回输出字符数。

`kern/libs/stdio.c:37-44` 的 `cprintf` 使用可变参数宏取得 `va_list`，调用 `vcprintf` 完成格式化输出，再调用 `va_end` 清理参数列表。它是内核代码最常用的格式化输出入口。

### 3.2 `vprintfmt`

`libs/printfmt.c:117-275` 的 `vprintfmt` 逐字符扫描格式字符串：

1. 普通字符直接调用 `putch(ch, putdat)`；
2. `%c`、`%s`、`%d`、`%u`、`%o`、`%x`、`%p` 等格式由对应分支解析；
3. 数字格式最终调用 `printnum`，由 `printnum` 继续通过 `putch` 输出每个字符；
4. 在 `vcprintf` 场景中，`putch` 实际上就是 `cputch`。

因此，格式化字符串、参数解析和字符发送是分开的：`vprintfmt` 决定“输出哪些字符”，`cputch` 决定“如何把单个字符送到 console”。

### 3.3 `cputch` 与 `cons_putc`

`kern/libs/stdio.c:11-14` 的静态函数 `cputch` 先调用 `cons_putc(c)`，再把计数器加一。计数器用于返回本次格式化输出产生的字符数。

`kern/driver/console.c:14` 的 `cons_putc` 把 `int` 转换为 `unsigned char`，调用 `sbi_console_putchar`。这一层隔离了上层 stdio 与底层 SBI 接口；上层不需要知道字符最终由哪一种固件服务输出。

### 3.4 SBI 接口与 `ecall`

`libs/sbi.h:17` 声明 `sbi_console_putchar(unsigned char ch)`。

`libs/sbi.c:7` 将传统 SBI console putchar 服务编号设为 `1`。`sbi_console_putchar`（`libs/sbi.c:32-34`）调用 `sbi_call(SBI_CONSOLE_PUTCHAR, ch, 0, 0)`。

`sbi_call`（`libs/sbi.c:16-30`）通过内联 RISC-V 汇编设置 SBI 调用约定中的寄存器：

- `x17`（`a7`）：SBI 服务编号；
- `x10`（`a0`）：第一个参数，本次为要输出的字符；
- `x11`（`a1`）、`x12`（`a2`）：第二、第三个参数，本次置零；
- `ecall`：从当前特权级陷入固件处理流程；
- `x10`（`a0`）：保存 SBI 调用返回值。

在 QEMU 的 RISC-V 虚拟机中，`ecall` 进入 OpenSBI 的机器模式固件。OpenSBI 根据服务编号和参数执行 console putchar，再把字符送到 QEMU 配置的串口或终端输出。内核侧只负责遵守 SBI 调用约定，不直接操作 QEMU 设备寄存器。

## 4. 关键不变量与边界

- `vprintfmt` 只依赖 `putch` 回调，因此可以复用于 console 输出和内存缓冲区输出；`snprintf` 使用的是 `sprintputch`，不经过 SBI。
- `cputch` 每发送一个字符增加一次计数；格式化填充、符号和换行也会分别计入。
- `cons_putc` 将 `int` 收窄为一个字节，适合当前字符输出接口；调用者应传入有效字符值。
- `sbi_call` 使用 `a7/a0-a2` 传递传统 SBI 调用信息，`ecall` 是进入 OpenSBI 的边界。
- Lab1 的 `sbi_console_putchar` 不返回错误码；本层无法直接报告固件输出失败。

## 5. 建议的验证记录

以下内容在实际运行后补入总报告，不应把未执行的命令写成已通过：

```sh
cd OS2026/code/labcodes/lab1
make clean
make
make qemu
```

WSL 中运行依赖 QEMU 的命令前先选择课程要求的版本：

```sh
use-qemu 4.1.1
make qemu
```

报告应记录：

- `make` 的编译结果；
- `make qemu` 中由 `cprintf` 等路径产生的实际输出；
- 截图文件名及其在 `report/images/` 中的相对链接；
- 若工具链、QEMU 或 OpenSBI 缺失，记录环境限制和未验证部分。

## 6. 交付整合清单

最终合并到 `OS2026/lab1` 分支时检查：

- `code/` 中包含完成后的 Lab1 源码；
- `report/report.md` 包含本节的 SBI/console 分析；
- `report/prompt.md` 汇总三位成员实际使用过的全部提示词；
- `report/images/` 只包含报告实际引用的运行或 GDB 截图；
- 从仓库根目录检查分支、`git diff --check` 和 `git status`；
- 不提交构建产物、临时日志、压缩包或本地工具配置。

## 7. 待成员 C 补充的信息

- 成员 C 的真实姓名和学号；
- 实际使用的 AI 工具和模型版本；
- `make`、`make qemu`、`make grade` 的实际输出；
- 运行截图和 GDB 截图；
- 最终整合时采用的 commit ID。

## 8. 截图操作指南

截图统一保存到最终仓库的 `OS2026/report/images/`，建议使用下面的文件名。Windows 下可用 `Win+Shift+S` 截取当前终端窗口；截图前先执行 `clear`，保证命令、关键输出和当前目录同时可见。

### 8.1 截图一：WSL、工具链和 QEMU 版本

打开 WSL 终端，执行：

```sh
cd '/mnt/d/code_warehouse/Curriculum/Computer System/OS2026/code/labcodes/lab1'
printf 'PWD: '; pwd
use-qemu 4.1.1
command -v riscv64-unknown-elf-gcc
```

截取显示 `PWD`、`已切换到 QEMU 4.1.1`、QEMU 版本和 RISC-V 编译器路径的终端画面，保存为：

```text
OS2026/report/images/member-c-qemu-version.jpg
```

### 8.2 截图二：编译成功

在同一目录执行：

```sh
clear
use-qemu 4.1.1
make clean
make
```

等待命令结束后截取最后一屏，至少包含编译命令、链接 `bin/kernel` 和生成 `bin/ucore.img` 的输出。保存为：

```text
OS2026/report/images/member-c-build-success.jpg
```

### 8.3 截图三：`make qemu` 控制台输出

执行：

```sh
clear
use-qemu 4.1.1
make qemu
```

截取包含 `(THU.CST) os is loading ...` 的 QEMU 终端画面。该命令会持续运行；截图完成后按 `Ctrl+A`，再按 `X` 退出 QEMU。保存为：

```text
OS2026/report/images/member-c-make-qemu.jpg
```

这张图证明内核已经通过 `cprintf` 输出到 QEMU 控制台，但不要仅凭它声称已经观察到每一层函数调用；函数链证据使用下一组 GDB 截图。

### 8.4 截图四：GDB 停在 `cons_putc` 的调用栈

需要两个 WSL 终端，且都进入同一个 Lab1 目录。

**终端 A：**

```sh
cd '/mnt/d/code_warehouse/Curriculum/Computer System/OS2026/code/labcodes/lab1'
use-qemu 4.1.1
make debug
```

终端 A 会停在等待 GDB 的状态，不要关闭。

**终端 B：**

```sh
cd '/mnt/d/code_warehouse/Curriculum/Computer System/OS2026/code/labcodes/lab1'
use-qemu 4.1.1
make gdb
```

进入 `(gdb)` 后执行：

```gdb
set pagination off
break cons_putc
continue
bt
info registers a0
```

当 GDB 停在 `cons_putc` 后，截取同时包含断点位置、`bt` 调用栈和 `a0` 字符参数的画面。调用栈应能看到 `cons_putc`、`cputch`、`vprintfmt`、`vcprintf`、`cprintf` 或 `kern_init` 中的连续部分。保存为：

```text
OS2026/report/images/member-c-gdb-cons-putc.jpg
```

### 8.5 截图五：GDB 停在 SBI 调用边界

在终端 B 的 GDB 中继续执行：

```gdb
break sbi_console_putchar
continue
bt
break sbi_call
continue
bt
info registers a0 a1 a2 a3
disassemble /m sbi_call
```

截取包含 `sbi_console_putchar`、`sbi_call` 和 `bt` 调用栈的画面，保存为：

```text
OS2026/report/images/member-c-ecall-disassembly.jpg
```

如需展示 `ecall` 指令，先在 `disassemble /m sbi_call` 输出中找到 `ecall` 对应地址，再执行：

```gdb
break *<ecall对应地址>
continue
x/i $pc
info registers a0 a7
```

截取 `x/i $pc` 显示 `ecall` 且 `a7=1` 的画面，保存为：

```text
OS2026/report/images/member-c-ecall-disassembly.jpg
```

`a7=1` 对应本实验使用的传统 SBI `console_putchar` 服务号；截图中的寄存器值必须以实际 GDB 输出为准。

### 8.6 截图后的清理和引用

停止 GDB 后，在终端 A 按 `Ctrl+C` 结束 QEMU。确认截图文件已经复制到 `OS2026/report/images/`，再在 `report/report.md` 中使用：

```markdown
![QEMU 4.1.1 和工具链](./images/member-c-qemu-version.jpg)
![Lab1 编译成功](./images/member-c-build-success.jpg)
![make qemu 控制台输出](./images/member-c-make-qemu.jpg)
![cons_putc 调用栈](./images/member-c-gdb-cons-putc.jpg)
![SBI 调用边界](./images/member-c-ecall-disassembly.jpg)
![ecall 指令和寄存器](./images/member-c-ecall-disassembly.jpg)
```

截图中的输出必须来自实际运行。当前目录没有 `tools/grade.sh`，因此不要制作“`make grade` 通过”的截图；如执行该命令失败，可单独记录为环境限制。
