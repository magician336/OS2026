# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 2：物理内存和页表 |
| **小组成员** | 2411046-梁家瑞、2412727-刘蔚霖、2411041-李培涛 |
| **完成日期** | 2026-10-22（成员 C 集成记录） |

### 小组分工

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2411046-梁家瑞（成员 A） | `page_init()`、First-Fit 分析、Buddy System 及测试 |
| 2412727-刘蔚霖（成员 B） | Best-Fit、SLUB 及测试 |
| 2411041-李培涛（成员 C） | 接口集成、构建验证、报告和交付检查 |

## 一、实验目的

本实验围绕物理内存的发现、初始化和分配展开，目标是：

1. 从设备树获取物理内存范围并建立 `Page` 描述符布局。
2. 理解 First-Fit 的连续物理页分配、释放和相邻块合并。
3. 在统一的 `pmm_manager` 接口下实现并接入 Best-Fit 和 Buddy System。
4. 在页级分配器之上实现 SLUB 任意大小对象分配。
5. 记录真实提示词、测试命令、验证证据和环境限制。

## 二、实验环境

| 成员 | 环境与工具 | 验证边界 |
|------|------------|----------|
| 成员 A | Windows + WSL Ubuntu；目标为 RISC-V 64 | 具体工具版本以成员 A 记录为准 |
| 成员 B | macOS、`riscv64-elf-gcc 16.2.0`、QEMU 11.1.2 | 提供了 Best-Fit、SLUB 和课程评分实测记录 |
| 成员 C | Windows + WSL Ubuntu 24.04、RISC-V GCC 15.1.0、QEMU 4.1.1 | 通过显式选择课程要求的 QEMU 版本后完成复核 |

代码目录为 `code/labcodes/lab2/lab2_new`，目标架构为 RISC-V 64，默认模拟物理内存为 128 MiB。各成员使用的 AI 工具和底层模型未在本次交付中统一确认，保留待填写，不据此推断。

## 三、实验整体逻辑分析

### 3.1 逻辑主线

启动页表建立后，内核先由 `init_pmm_manager()` 选择页级物理内存管理器。`page_init()` 读取设备树提供的物理内存范围，设置 `va_pa_offset`、`npage` 和 `pages`，排除 OpenSBI、内核镜像、`Page` 描述符数组和对齐占用的页面，再通过 `init_memmap()` 注册完整空闲页。

上层通过统一的 `alloc_pages()`、`free_pages()` 和 `nr_free_pages()` 使用不同的页级算法。First-Fit 作为参考实现，Best-Fit 选择最小的可用空闲块，Buddy System 按二的幂次分裂和合并。SLUB 位于页级分配器之上，把页面切成固定规格对象；大对象则申请连续多页。

### 3.2 实现顺序

1. 设备树初始化提供物理内存基址和大小。
2. `page_init()` 建立可管理页面和初始空闲区。
3. 阅读 First-Fit，固定 `PageReserved`、`PageProperty`、`property` 和 `nr_free` 等状态约定。
4. 接入 Best-Fit 和 Buddy System，并保持 `pmm_manager` 接口一致。
5. 初始化 SLUB，验证小对象、大对象、对齐、复用和释放。
6. 集成三人的代码，检查报告、提示词、截图和生成物。

## 四、实验内容与实现

### 4.1 物理内存探测与 Page 描述符初始化

**负责人：** 2411046-梁家瑞

**涉及文件和函数：**

- `kern/mm/pmm.c`：`page_init()`、`pmm_init()`、`init_pmm_manager()`；
- `kern/driver/dtb.c`：`get_memory_base()`、`get_memory_size()`；
- `kern/mm/memlayout.h`：地址转换和页布局宏。

`page_init()` 先检查设备树范围是否有效，再设置虚实地址偏移和页数上界。`pages` 在内核 `end` 之后按页对齐；描述符数组的物理末端使用 `PADDR()` 计算，空闲起点向上对齐，设备树物理内存末端向下对齐。只有存在完整页面时才调用 `init_memmap()`。

成员 A 的记录覆盖了设备树无效、地址加法溢出、物理内存上界、描述符数组占用和空闲区不足一页等边界。成员 A 提供的实测截图见 [page_init 验证](./images/lab2-A-page-init.png)。

本模块的提示词记录见 `report/prompt.md` 的 Prompt A-1；该提示词只用于整理报告，没有生成或修改源码。

### 4.2 First-Fit 原理分析

**负责人：** 2411046-梁家瑞

`default_init()` 初始化空闲链表和 `nr_free`；`default_init_memmap()` 将连续页面登记为一个空闲块；`default_alloc_pages()` 从地址有序链表中选择第一个足够大的块并在必要时切分；`default_free_pages()` 插入释放区并合并相邻块。

First-Fit 实现简单，按地址排序也方便合并，但可能线性扫描空闲块并造成外部碎片。可选改进包括 Best-Fit、按大小分组的空闲链表和 Buddy System，各自需要在搜索开销、内部碎片、外部碎片和实现复杂度之间取舍。

First-Fit 的说明和改进分析与 `report/prompt.md` 的 Prompt A-1 一起记录。

### 4.3 Best-Fit 连续页分配器

**负责人：** 2412727-刘蔚霖

**涉及文件和函数：**

- `kern/mm/best_fit_pmm.c`：初始化、空闲区登记、最小可用块选择、切分、释放、合并和附加检查；
- `kern/mm/pmm.c`：根据 `USE_BEST_FIT_PMM` 选择 `best_fit_pmm_manager`；
- `kern/mm/default_pmm.c`：修复尾部插入后缺少 `break` 的问题。

Best-Fit 遍历全部空闲块，选择满足请求的最小块；同尺寸候选保持地址较低者。分配时保留剩余块，释放时按地址插入并合并前后相邻块，`nr_free` 只按实际页数变化。

成员 B 的附加测试覆盖最小适配、同尺寸选择、切分、连续块不足、前后合并和链表计数。成员 B 记录的完整验证截图见 [Best-Fit 与 SLUB](./images/lab2-B-best-fit-slub.png) 和 [完整检查](./images/lab2-B-full-checks.png)。

成员 B 的实现提示词和迭代记录见 `report/promptB.md`；成员 C 的集成核验提示词见 `report/prompt.md` 的 Prompt C-1。

### 4.4 Buddy System

**负责人：** 2411046-梁家瑞

**涉及文件和函数：**

- `kern/mm/buddy_pmm.c`：`buddy_init()`、`buddy_init_memmap()`、`buddy_alloc_pages()`、`buddy_free_pages()`、`buddy_check()`；
- `kern/mm/buddy_pmm.h`：管理器声明；
- `kern/mm/pmm.c`：`USE_BUDDY_PMM` 选择路径。

Buddy System 使用不同阶次的空闲链表。初始化时把不一定按最大块对齐的区间分解成多个二的幂次块；分配时向上取整到目标阶次，从更高阶链表取得块后不断分裂；释放时通过相对页号异或定位伙伴，检查边界和阶次后逐级合并。

成员 A 的 `buddy_check()` 覆盖 1、3、8 页请求、分裂、耗尽失败、不同释放顺序、两级伙伴合并、释放后重新分配和空闲页计数恢复。实测截图见 [Buddy 检查](./images/lab2-A-buddy-check.png)。

Buddy System 的实现和测试说明提示词见 `report/prompt.md` 的 Prompt A-2 和 Prompt A-3。

### 4.5 SLUB 任意大小对象分配

**负责人：** 2412727-刘蔚霖

**涉及文件和函数：**

- `kern/mm/slub.c`：cache、slab、对象分配、对齐、释放和大对象路径；
- `kern/mm/slub.h`：`slub_init()`、`kmalloc()`、`kmalloc_aligned()`、`kfree()`、`slub_check()`；
- `kern/mm/slub_test.c`：正常路径、耗尽和非法释放测试；
- `kern/init/init.c`：初始化后调用 SLUB 自检。

小对象使用 16 至 2048 字节的规格 cache。每个 slab 从页级分配器取得页面，将元数据、对齐填充和对象槽位放在页面中；空闲对象通过链表和位图管理。释放对象时恢复槽位，空 slab 归还后端页面。超过小对象规格的请求使用连续多页，并检查溢出、对齐和空间不足。

成员 B 的自检覆盖 30 种大小、跨 slab 分配、释放复用、对齐、大对象、算术溢出、真实耗尽和五类非法释放。课程评分截图见 [Lab2 grade](./images/lab2-B-grade.png)。

SLUB 的实现、测试和错误路径提示词记录见 `report/promptB.md`。

### 4.6 成员 C：联调、验证与交付整理

**负责人：** 2411041-李培涛

成员 C 没有重复实现成员 A、B 已完成的分配算法，主要负责把三人的工作放到同一份 `lab2` 代码基线上，并用实际运行结果确认它们能够共同工作。

#### 代码与接口联调

1. 拉取远程 `lab2` 分支的最新提交，核对成员 A 的 `page_init()`/Buddy System 与成员 B 的 Best-Fit/SLUB 是否同时存在。
2. 梳理 `pmm_init()` → `init_pmm_manager()` → `page_init()` → `init_memmap()` → `alloc_pages()`/`free_pages()` 的调用链，确认三种页级后端都遵守 `pmm_manager` 接口。
3. 检查 `USE_BEST_FIT_PMM`、`USE_BUDDY_PMM` 后端选择、SLUB 初始化入口和 Makefile 的编译参数，避免一个后端的改动覆盖其他后端。
4. 检查构建生成物和临时文件的忽略规则，避免 `bin/`、`obj/`、日志等文件进入交付目录。

#### 构建和运行验证

成员 C 在 WSL2 中选择 QEMU 4.1.1 并加载 RISC-V 工具链后执行：

```sh
make clean
make GCCPREFIX=riscv64-unknown-elf- DEFS=-DUSE_BEST_FIT_PMM
make GCCPREFIX=riscv64-unknown-elf- QEMU=qemu-system-riscv64 DEFS=-DUSE_BEST_FIT_PMM qemu
make GCCPREFIX=riscv64-unknown-elf- QEMU=qemu-system-riscv64 DEFS=-DUSE_BEST_FIT_PMM grade
python3 tools/check_member_b.py \
  --gccprefix riscv64-unknown-elf- \
  --qemu qemu-system-riscv64 \
  --timeout 30
```

验证结果为：

- 内核成功编译并生成镜像；
- QEMU 启动后输出 `best_fit_pmm_manager`，Best-Fit 检查和四组 SLUB 自检均为 `PASS`；
- `make grade` 输出 `page init: OK`、`best fit: OK` 和 `Total Score: 20/20`；
- 自动验证脚本覆盖 Best-Fit、First-Fit、Buddy 三种后端，以及五类非法释放路径，全部通过。

#### 报告和交付整理

1. 将成员 A、B 的阶段报告合并为 `report/report.md`，补齐实验目的、整体逻辑、模块接口、Challenge、测试结果和限制说明。
2. 汇总成员 A 的 Prompt A-1/A-2/A-3、成员 B 的 `promptB.md`，并补充成员 C 的集成核验提示词。
3. 检查 `report/images/` 中的截图文件与 Markdown 链接一一对应，保留截图来源和验证边界。
4. 执行 `git diff --check`、`git status` 和分支/远程核对，提交并推送最终 `lab2` 分支。

成员 C 的交付重点是让算法实现、测试证据、报告和 Git 分支形成可复核闭环；算法正确性结论仍以成员 A、B 的代码和三人共同复跑的测试结果为依据。

### 4.7 Challenge：未知硬件的可用物理内存范围

**负责人：** 2411046-梁家瑞

操作系统通常通过固件或引导程序提供的硬件描述获得内存布局。RISC-V 可解析设备树 `memory` 节点，其他平台可使用 BIOS E820、UEFI memory map 或 ACPI。内核获得范围后还要扣除固件、内核镜像、设备保留区、内存映射 I/O 和自身元数据。若平台完全没有描述接口，破坏性探测不应作为通用操作系统的首选方案。

## 五、集成、测试与验证

### 5.1 成员 B 提供的已完成验证

成员 B 记录了以下本机实测：

| 测试 | 结果来源 |
|------|----------|
| Best-Fit + SLUB | 成员 B 终端截图和 `promptB.md` 记录为 PASS |
| First-Fit + SLUB | 成员 B 自动验证记录为 PASS |
| Buddy + SLUB | 成员 B 自动验证记录为 PASS |
| 非法释放路径 | 预期 `panic`，成员 B 记录为通过 |
| 课程评分 | `make ... grade` 记录为 20/20，见 [评分截图](./images/lab2-B-grade.png) |

这些结论最初由成员 B 提供环境和截图；成员 C 随后在 WSL2 中使用课程要求的 QEMU 4.1.1 和 RISC-V 工具链重跑了同一套检查，结果一致。

### 5.2 成员 C 当前环境验证

成员 C 在 WSL2 Ubuntu 24.04 选择 QEMU 4.1.1 并加载 RISC-V 工具链后执行了：

```sh
make
make qemu
make grade
```

构建成功生成内核镜像；`make qemu` 输出设备树物理内存范围、`best_fit_pmm_manager`、Best-Fit 检查和四组 SLUB 自检均为 `PASS`。课程评分命令输出 `page init: OK`、`best fit: OK` 和 `Total Score: 20/20`。自动验证脚本另外报告以下结果：

- Best-Fit + SLUB：PASS；
- First-Fit + SLUB：PASS；
- Buddy + SLUB：PASS；
- 小对象内部地址、重复释放、外来地址、大对象内部页和大对象重复释放：均按预期触发 `panic`；
- course grade：20/20。

### 5.3 交付检查

- 当前分支为 `lab2`，远程跟踪 `origin/lab2`。
- `bin/`、`obj/`、日志和其他本地生成物已加入忽略规则。
- 报告引用的 lab2 图片均位于 `report/images/`。
- A、B 的阶段报告内容已合并到本报告，提示词记录仍保留在 `prompt.md` 和 `promptB.md` 中。
- 代码和报告的最终构建、QEMU、课程评分及成员 B 全套检查均已在 WSL2 完成。

## 六、实验总结与收获

本实验把物理内存从设备树描述转换为可管理的 `Page` 资源，再用统一接口替换不同分配策略。First-Fit 展示了连续空闲块链表的基本语义；Best-Fit 改变了选块策略；Buddy System 通过二的幂次分裂和合并改善了伙伴定位；SLUB 则在页级分配之上提供对象级复用。

实现过程中最重要的不变量包括：保留页不能进入空闲区，空闲块头的 `property` 必须与链表状态一致，`nr_free` 必须等于所有空闲块页数之和，SLUB 释放时不能接受对象内部地址或重复释放。成员 B 的测试还说明，页级课程评分不能代替 SLUB 的对象级和非法释放测试。

本次集成在 WSL2 中完成了课程构建、QEMU、grade 和成员 B 全套检查。仍需注意该实现是单核教学版本，报告只把成员 A/B 的截图和成员 C 的实际终端输出分别作为证据，不用课程 grade 代替 SLUB 的对象级测试。
