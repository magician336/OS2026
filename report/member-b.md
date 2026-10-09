# Lab 1 成员 B：启动流程与 GDB 验证

## 实验环境与复现方法

- 日期：2026-10-08 至 2026-10-09；主机：Apple Silicon macOS；代码：`OS2026` 仓库 `lab1` 分支（开始实验时提交 `6d28823`）。
- 工具：Homebrew 的 `riscv64-elf-gcc` 16.2.0、`riscv64-elf-gdb` 18.1、QEMU 11.1.2（自带 OpenSBI 1.8.1）。
- 在 `code/labcodes/lab1` 下执行 `make GCCPREFIX=riscv64-elf-`，成功生成 `bin/kernel`（ELF，入口 `0x80200000`）和 `bin/ucore.img`（裸二进制镜像）。

本机的 QEMU 11.1.2 使用仓库原有的 `-device loader,file=bin/ucore.img,addr=0x80200000` 时，OpenSBI 显示 `Domain0 Next Address : 0x0`，不会进入内核。改用 `-kernel bin/ucore.img` 后，下一阶段地址为 `0x80200000`，并输出 `(THU.CST) os is loading ...`。这是一项需要成员 A 处理的运行参数兼容性问题；以下调试使用可运行的 `-kernel` 参数，不改动成员 A 负责的 Makefile。

在普通终端中，可用两个窗口复现：

```sh
# 终端 1，位于 code/labcodes/lab1
qemu-system-riscv64 -machine virt -nographic -bios default -kernel bin/ucore.img -s -S

# 终端 2，位于同一目录
riscv64-elf-gdb bin/kernel
(gdb) target remote localhost:1234
```

10 月 9 日在 macOS 终端中用上面的两个窗口和 `localhost:1234` 完成验证，并拍摄下文的真实终端截图。使用的 GDB 命令包括 `info registers pc sp`、`x/6i $pc`、`si 6`、`break *0x80200000`、`continue`、`p/x &bootstacktop` 和 `si`。

## 练习 1：理解内核入口

`kern/init/entry.S` 定义全局符号 `kern_entry`。链接脚本 `tools/kernel.ld` 以 `ENTRY(kern_entry)` 指定 ELF 入口，并从 `BASE_ADDRESS = 0x80200000` 开始放置代码。`readelf -h bin/kernel` 显示入口确为 `0x80200000`，GDB 也在该地址命中 `kern_entry`。

- `la sp, bootstacktop` 把内核栈顶地址加载到栈指针 `sp`。源码在 `.data` 段为 `bootstack` 预留 `KSTACKSIZE = 2 × 4096 = 8192` 字节，栈从高地址向低地址增长。本次实测 `bootstack = 0x80201000`、`bootstacktop = 0x80203000`；执行该伪指令展开的两条机器指令后，`sp` 从 OpenSBI 阶段的 `0x80045e30` 变为 `0x80203000`。这使随后的 C 函数调用使用内核自己的栈。
- `tail kern_init` 是尾调用跳转，把执行权交给 C 函数 `kern_init`，不为 `kern_entry` 建立新的返回帧，也不更新返回地址。实测它在 `0x80200008` 编码为跳转，单步后 PC 到达 `kern_init = 0x8020000a`。`kern_init` 清理 BSS、打印启动信息并进入无限循环，因此启动路径不返回 `kern_entry`。

![GDB 命中 kern_entry，显示旧 sp 与 bootstacktop](./images/gdb-entry.png)

![单步执行 la sp 后，sp 变为 0x80203000](./images/gdb-stack.png)

## 练习 2：GDB 验证启动流程

1. GDB 连接暂停的 QEMU 时，`pc = 0x1000`、`sp = 0x0`。反汇编显示最初六条指令位于 QEMU 的复位 ROM：`auipc t0,0x0`、`addi a2,t0,40`、`csrr a0,mhartid`、两条 `ld`、`jr t0`。它们读取当前 hart 编号和 ROM 中的启动参数，再跳转到固件入口。
2. 从 `0x1000` 连续执行六条指令后，`pc = 0x80000000`，开始执行 OpenSBI。启动输出也报告 `Firmware Base : 0x80000000`。OpenSBI 完成机器级初始化，准备进入 S 模式的下一阶段。
3. 在 `0x80200000` 设置断点并继续，GDB 命中 `kern_entry`。OpenSBI 输出的 `Domain0 Next Address` 同为 `0x80200000`。此时 `sp` 仍指向固件使用的地址；单步执行 `la sp, bootstacktop` 后才切到内核栈，再经 `tail kern_init` 进入 C 入口。

![GDB 从复位地址 0x1000 单步到 OpenSBI 入口 0x80000000](./images/gdb-reset-opensbi.png)

**关于镜像加载的观察：**在 PC 仍为 `0x1000` 时，GDB 读取 `0x80200000` 已能看到 `kern_entry` 的机器码。就本次 `-kernel` 配置而言，QEMU 在 CPU 开始执行前已把内核镜像放到内存，OpenSBI 负责初始化并转交控制权；因此对 `0x80200000` 设写监视点并不适合观察“加载瞬间”。断点 `break *0x80200000` 能直接验证控制权交接。

## 供组长整合的结论与材料

- 练习 1、练习 2 均已通过本机实际调试完成，具体地址和寄存器值以本文实测为准，不沿用指导书截图中的旧版本数值。
- 仓库现有 `make debug` 的 QEMU 参数需由成员 A 核对 QEMU 11 兼容性；当前 `make gdb` 还写死了 `riscv64-unknown-elf-gdb`，而 Homebrew 提供的命令名是 `riscv64-elf-gdb`。
- 报告中的三张 PNG 配图是 10 月 9 日在 macOS 终端实际调试时截取的画面，分别证明复位地址到 OpenSBI、命中内核入口，以及内核栈设置。

参考：[Lab 1 练习](http://oslab.mobisys.top/lab2026/_book/lab1/lab1_2_1_exercise.html)、[GDB 指导](http://oslab.mobisys.top/lab2026/_book/lab1/lab1_4_gdb.html)、[链接脚本说明](http://oslab.mobisys.top/lab2026/_book/lab1/lab1_3_2_linkerscript.html)。
