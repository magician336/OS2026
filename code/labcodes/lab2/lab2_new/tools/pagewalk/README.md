# Lab2：双 GDB 观察 QEMU 的页表查询

负责人：2412727 刘蔚霖。实验结果和计算见仓库 `report/report.md` 的 4.8 节。

这里的两个 GDB 各有一个目标：Linux 原生 `gdb` 调试 QEMU 的宿主 C 程序，`gdb-multiarch` 通过 QEMU 的 1234 端口调试 RISC-V 内核。后者执行一条 `sd ra,8(sp)` 时，前者记录模拟 MMU 的页表查询、权限判断结果和 TLB 填充。

## 环境和适用范围

- 已实测：macOS Apple Silicon + Docker Ubuntu 24.04/aarch64，宿主 GCC 13.3.0、GDB 15.1、QEMU 4.1.1，内核由 `riscv64-elf-gcc 16.2.0` 编译。
- QEMU 用 `--enable-debug --disable-strip --extra-cflags=-O0` 构建，未修改 QEMU 的 MMU 算法；`--disable-werror` 允许旧源码在新编译器上的警告。
- Dockerfile 固定 QEMU 和 keycodemapdb 源码版本、校验下载摘要，并使用系统 libfdt。其安装、解压、配置和编译命令已在实验容器内逐条执行成功；本次实测没有另行执行 `docker build`。
- 脚本专门验证当前 Lab2 启动页表：Sv39、根表第 511 项为 1 GiB 叶子映射。它不是通用的所有页表配置测试器；换内核、QEMU 或映射后应重新选择断点并审查断言。
- 本实验不改内核源码。第二次写是调试器把 PC 放回同一条栈写指令后重放，写入值不变，用来对照缓存命中；它不是内核自然执行了两遍这条指令。

## 从头复现

在 `code/labcodes/lab2/lab2_new` 目录执行，先确认 Docker 已启动：

```sh
make clean
make GCCPREFIX=riscv64-elf- DEFS=-DUSE_BEST_FIT_PMM
docker build -t os2026-pagewalk:4.1.1 tools/pagewalk
evidence_dir="$(mktemp -d)"
docker run --rm --cap-add=SYS_PTRACE \
  -v "$PWD:/kernel:ro" -v "$evidence_dir:/evidence" \
  os2026-pagewalk:4.1.1 \
  python3 /kernel/tools/pagewalk/run.py \
  --qemu /opt/qemu-build/riscv64-softmmu/qemu-system-riscv64 \
  --source /opt/qemu-src --kernel-dir /kernel --out /evidence/run
```

Linux 上可换成本机交叉工具链前缀，例如 `riscv64-unknown-elf-`。构建 QEMU 需要下载依赖和源码。容器内部的 1234 端口只供两个调试器连接，无需向宿主机开放端口，也不需要 `--privileged`。

如果已经有 Linux 调试环境、QEMU 4.1.1 的带符号构建和内核，直接运行：

```sh
python3 tools/pagewalk/run.py \
  --qemu /opt/qemu-build/riscv64-softmmu/qemu-system-riscv64 \
  --source /opt/qemu-src --kernel-dir "$PWD" --out /tmp/pagewalk-new-run
```

`--out` 必须是新目录。原始证据保存在该目录的 `host.log`、`guest.log`、`console.log`、`host.json`、`guest.json`、`summary.json`；这些运行生成物不加入 Git。所有断言和内核自检成功后才输出 `"result": "PASS"`。失败时保留日志以便定位原因。

## 脚本具体做了什么

1. `run.py` 启动原生 GDB，并由它运行带 `-S -gdb tcp:127.0.0.1:1234` 的 QEMU；`-S` 让 CPU 等待调试器。
2. `guest.py` 连接 gdbstub，断在 `kern_init`，通过反汇编找到该函数的第一条 `sd ra,8(sp)`，记录 PC、SP、RA、SATP 和页表项。
3. `host.py` 排除调试器读内存产生的地址翻译，选择第一条启用 Sv39 后的高地址 **store**。在 `get_physical_address` 入口记录访问类型和调用栈；在 `cpu_helper.c:256` 记录已读出的 PTE；用 GDB `FinishBreakpoint` 记录翻译返回值、物理页地址与权限。
4. 在 `tlb_set_page` 和 `cputlb.c:836`（复制 TLB 项后）记录页映射、索引及读/写/执行标签。
5. 内层 GDB 单步执行两次同一条 store，确认两次 PC 都前进、写入值都等于 RA。再通过 `Qqemu.PhyMemMode:1` 直接读取计算所得物理地址，随后恢复虚拟地址调试模式。
6. 断言两次 store 对应一次目标地址查表、一次根表访问，页表计算与 QEMU 返回结果一致，TLB 写标签有效，实际物理内存写入正确。第二次未再次查表并结合有效 TLB 项，支持第二次写命中缓存的结论。
7. 脱离内层调试器，让内核完成 Best-Fit、SLUB 自检；最后停止本脚本启动的 GDB/QEMU 进程组。

宿主日志可能出现 `Error disabling address space randomization: Operation not permitted`：容器未允许关闭宿主 ASLR。本次按符号和源码行设置断点仍全部命中，不依赖固定宿主地址，无需为此放宽容器安全设置。

## 手动观察时的顺序

自动脚本是为了可重复记录。答辩时也可手动使用相同断点：

1. 外层原生 GDB 加载 QEMU ELF，设置 QEMU 的启动参数，并在 `get_physical_address` 设置条件断点：`access_type == 1 && (env->satp >> 60) == 8 && addr > 0xffffffffc0000000`，然后 `run`。
2. 内层 `gdb-multiarch bin/kernel` 执行 `target remote 127.0.0.1:1234`，断到 `kern_init` 的目标 store 前，查看 `x/i $pc` 和 `info registers pc sp ra satp`，执行 `si`。
3. 此时内层等待是正常现象：外层 GDB 把 QEMU 暂停了。切到外层，用 `bt`、`p/x addr`、`p/x env->satp` 看真实访存调用链，再断到 `target/riscv/cpu_helper.c:256` 查看 `idx`、`pte_addr`、`pte`、`ppn`、`ptshift`。
4. `finish` 回到 `riscv_cpu_tlb_fill`，查看 `pa`、`prot`；再断到 `tlb_set_page` 看 `vaddr`、`paddr`、`size`，随后继续运行。内层的 `si` 才能完成。

本次地址：指令 `0xffffffffc02000ec`，目标虚拟地址 `0xffffffffc0205ff8`，物理地址 `0x80205ff8`。重新编译后应以实际反汇编和符号为准，不照抄地址。

源码参考：[QEMU 4.1.1 RISC-V 地址翻译](https://github.com/qemu/qemu/blob/v4.1.1/target/riscv/cpu_helper.c)、[QEMU 4.1.1 软件 TLB](https://github.com/qemu/qemu/blob/v4.1.1/accel/tcg/cputlb.c)。
