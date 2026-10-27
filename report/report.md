# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 3：中断与异常处理 |
| **小组成员** | 2411046-梁家瑞、2412727-刘蔚霖、2411041-李培涛 |
| **完成日期** | 2026-10-27 |

### 小组分工

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2411046-梁家瑞（成员 A） | 功能模块一：Trap 上下文恢复与 Trap 入口（`kern/trap/trapentry.S` 的 `RESTORE_ALL`、`__alltraps`） |
| 2412727-刘蔚霖（成员 B） | 功能模块二：中断与异常处理（`kern/trap/trap.c` 的 `interrupt_handler()`、`exception_handler()`） |
| 2411041-李培涛（成员 C） | lab2 代码迁移、接口集成、构建验证、报告与交付检查 |

> 当前状态（2026-10-27）：M0 框架导入与 lab2 迁移已完成，`make` 编译通过；A/B 模块仍为 `lab3 function1/2 YOUR CODE` 占位，尚未实现，QEMU 全流程待 A/B 集成后验证。

---

## 一、实验目的

1. 理解 RISC-V 的 Trap 机制：中断、异常、`stvec`、`sepc`、`scause`、`sstatus` 与 `sret`。
2. 掌握 Trap 上下文的保存与恢复，理解 `SAVE_ALL` / `RESTORE_ALL` 与 `struct trapframe` 的对应关系。
3. 实现时钟中断、非法指令异常和断点异常的处理，并保证异常返回后不会重复执行同一条指令。
4. 继续使用 AI 辅助开发，记录提示词迭代、实现问题和验证证据。

---

## 二、实验环境

| 成员 | 环境与工具 | 验证边界 |
|------|------------|----------|
| 成员 A | [待填写] | [待填写] |
| 成员 B | [待填写] | [待填写] |
| 成员 C | Windows + WSL Ubuntu 24.04、RISC-V GCC 15.1.0、QEMU 4.1.1 | 当前完成 lab3 框架导入、lab2 迁移和编译验证 |

代码目录：`code/labcodes/lab3/lab3_new`，目标架构 RISC-V 64，默认 QEMU 物理内存 128 MiB。

---

## 三、实验整体逻辑分析

### 3.1 逻辑主线

Lab3 在 Lab2 的物理内存管理和页表基础上加入 Trap 处理：

`kern_init` → `dtb_init` → `idt_init`（`stvec = __alltraps`）→ `pmm_init` → `idt_init` → `clock_init` → `intr_enable` → 时钟中断持续触发。

Trap 发生后的链路：

`__alltraps` → `SAVE_ALL` → `trap(struct trapframe *)` → `trap_dispatch` → `interrupt_handler` / `exception_handler` → `__trapret` → `RESTORE_ALL` → `sret`。

### 3.2 实现顺序

1. **C（M0）**：导入 lab3 原始框架，合入 lab2 的 `page_init` 和 Best-Fit，保证框架可编译。
2. **A**：实现 `RESTORE_ALL` 和 `__alltraps`，完成上下文保存/恢复闭环。
3. **B**：实现时钟中断、非法指令异常和断点异常处理。
4. **C（M3）**：联调、构建、QEMU/grade 验证、报告和交付检查。

---

## 四、实验内容与实现

### 4.1 功能模块一：Trap 上下文恢复与 Trap 入口

**负责人：** 2411046-梁家瑞

**需要实现/修改的函数：**

```asm
RESTORE_ALL
__alltraps
```

**实现说明：** [待成员 A 填写]

**最终提示词：** [待成员 A 填写]

**迭代与验证：** [待成员 A 填写：GDB 上下文核对、断点异常闭环]

---

### 4.2 功能模块二：中断与异常处理

**负责人：** 2412727-刘蔚霖

**需要实现/修改的函数：**

```c
void interrupt_handler(struct trapframe *tf);
void exception_handler(struct trapframe *tf);
```

**实现说明：** [待成员 B 填写]

**最终提示词：** [待成员 B 填写]

**迭代与验证：** [待成员 B 填写：时钟中断统计、关机、非法指令/断点异常 epc 更新]

---

### 4.3 成员 C：lab2 迁移、集成与构建

**负责人：** 2411041-李培涛

#### 框架导入

- 将 lab3 原始代码导入 `code/labcodes/lab3/lab3_new`。
- 保留 `kern/trap/trapentry.S` 的 `RESTORE_ALL` / `__alltraps` 占位，交由成员 A 实现。
- 保留 `kern/trap/trap.c` 的 `lab3 function2 YOUR CODE` 占位，交由成员 B 实现。

#### lab2 迁移

- `kern/mm/pmm.c`：合入 lab2 的 `page_init()`，设置 `va_pa_offset`、`npage`、`pages`，排除内核和描述符数组，将完整空闲页交给 `init_memmap()`。
- `kern/mm/pmm.c`：默认选择 `best_fit_pmm_manager`；保留 `USE_FIRST_FIT_PMM` 宏用于 First-Fit 对照。
- `kern/mm/best_fit_pmm.c`：合入 lab2 的 Best-Fit 实现与附加测试。
- `kern/mm/default_pmm.c`：补齐尾部插入后的 `break`，避免链表遍历继续。
- `kern/mm/memlayout.h`：补充 `PHYSICAL_MEMORY_END`。
- `kern/trap/trapentry.S`：把旧 CSR 名 `sbadaddr` 改为当前工具链支持的 `stval`。
- `kern/trap/trap_stub.c`：新增弱符号占位，保证 A/B 实现前框架可链接；B 的强符号会覆盖。

#### 当前构建结果

```sh
cd code/labcodes/lab3/lab3_new
make GCCPREFIX=riscv64-unknown-elf-
```

结果：编译、链接和 `objcopy` 通过，生成 `bin/kernel` 与 `bin/ucore.img`。

QEMU 启动到时钟初始化阶段，输出：

```text
memory management: best_fit_pmm_manager
[Best-Fit extra] choice/tie/split/fragmentation/merge: PASS
check_alloc_page() succeeded!
++ setup timer interrupts
```

说明 lab2 的物理内存初始化和 Best-Fit 已经接入 lab3；时钟中断之后的 Trap 处理仍依赖 A/B 的实现。

**边界：** 当前 `RESTORE_ALL`、`__alltraps` 和两个 handler 尚未实现，因此不能把 M0 构建通过当成 Lab3 功能通过；QEMU 全流程和 `make grade` 待 A/B 集成后执行。

---

## 五、测试与验证

### 5.1 已完成

| 验证 | 命令 | 结果 |
|------|------|------|
| 框架编译 | `make GCCPREFIX=riscv64-unknown-elf-` | 通过 |
| lab2 迁移静态检查 | 对照 `page_init` / `best_fit_pmm.c` | 已合入 |
| QEMU 启动到时钟初始化 | `qemu-system-riscv64 ... -device loader,file=bin/ucore.img,addr=0x80200000` | 通过：Best-Fit PASS、`++ setup timer interrupts`；后续 Trap 待 A/B |

### 5.2 待完成

- [ ] 成员 A：`RESTORE_ALL` / `__alltraps` 的 GDB 上下文验证、断点异常闭环。
- [ ] 成员 B：时钟中断统计与关机、非法指令/断点异常 epc 更新。
- [ ] 成员 C：`make qemu` 全流程、`make grade`、截图归档。
- [ ] 三人共同复核报告、`prompt.md` 和交付目录。

---

## 六、实验总结与收获

### 对操作系统的理解

[待集成完成后填写：Trap frame 布局、上下文保存恢复、异常返回地址更新、时钟中断与调度的关系。]

### AI 协作开发的经验

[待各成员补充：提示词迭代、AI 生成代码的问题、如何验证。]

---

## 七、报告要求对照

| 要求 | 对应位置 | 状态 |
|------|----------|------|
| 整体逻辑线 | 三 | 已写骨架 |
| 核心函数/模块理解 | 四 | A/B 待填 |
| 每个练习的 spec 与 prompt | 四 + `prompt.md` | 待填 |
| AI 生成代码的 bug 与解决 | 四 | 待填 |
| 重要知识点与 OS 原理对照 | 六 | 待填 |
| OS 原理中未对应的知识点 | 六 | 待填 |
| 测试与截图 | 五 + `images/` | 待补 |
