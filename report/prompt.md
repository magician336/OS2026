# Lab1 成员 B Prompt 汇总

以下按主题合并整理成员 B 在本次实验会话中实际提出的提示词和补充要求，并非逐字转录。成员 B 的独立报告为 `report/member-b.md`，GDB 终端截图位于 `report/images/`。

## Prompt 1：阅读实验材料并完成成员 B 的任务

```text
查看 http://oslab.mobisys.top/lab2026/_book/，告诉我 Lab1 要做什么。我负责成员 B：启动流程和 GDB，需要完成 PDF 中 Lab1 的两个练习，观察最初停在 0x1000，再断到 kern_entry 或 0x80200000，解释 la sp, bootstacktop 和 tail kern_init，并记录从复位地址经 OpenSBI 到内核入口的过程，配真实截图。先阅读所有材料，评估当前电脑完成实验的可行性，给出计划，然后开始完成。
```

## Prompt 2：限定工作目录和交付方式

```text
所有代码内容在 txjliu/2026fall/OS 中完成。把自己的报告写在独立的 md 文件里，后面组长整理；推送到 lab1 分支，它要求保留 git 记录。
```

## Prompt 3：定位本机运行环境问题

```text
确认需要哪些同学完成哪些前置任务，我去交涉。解释本机 QEMU 11 与 Makefile 启动参数的兼容问题：现有 -device loader 无法进入内核，改用 -kernel 已验证可运行；还需要让 make gdb 支持本机的 riscv64-elf-gdb，并验证 make qemu 和 make debug。
```

## Prompt 4：指导取得真实 GDB 终端截图

```text
怎么获取合格真实终端截图？给我操作，我来补图。请依据我提供的终端截图，确认是否记录了 PC 从 0x1000 到 OpenSBI 的 0x80000000、在 0x80200000 命中 kern_entry，以及执行 la sp, bootstacktop 后 sp 变为 0x80203000。
```

## Prompt 5：完成、提交并核对小组整合

```text
完成成员 B 的所有内容并提交，然后把提交内容给我审核。看下成员 A 的提交在 GitHub 的 lab1 分支是否已经提交；prompt 也要提交，整合一下。和成员 A 一样的提交规范，修复连接超时的问题然后推送 GitHub。
```

**使用与核查：** AI 辅助阅读实验材料、分析启动流程、解释 GDB 输出、整理独立报告和截图以及核对 Git 提交。报告中的地址、寄存器值和运行结果以真实终端记录为准；QEMU/GDB 的本机兼容修改仍由负责构建运行的成员 A 核对。
