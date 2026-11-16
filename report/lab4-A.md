# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| 实验名称 | Lab 4：进程管理——成员 A：虚拟内存页操作 |
| 小组成员 | 2411046-梁家瑞、2412727-刘蔚霖、2411041-李培涛 |
| 本人负责 | 2411046-梁家瑞：page_insert、page_remove、Challenge 2 |
| 代码目录 | code/labcodes/lab4/lab4_new |
| 完成日期 | 2026-11-16 |

### 小组分工

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2411046-梁家瑞 | 虚拟内存页操作：page_insert、page_remove；Challenge 2 |
| 2412727-刘蔚霖 | 进程创建及初始化 |
| 2411041-李培涛 | 进程调度、Challenge 1、接口集成与交付 |

本报告只详细记录成员 A 的代码和理解。

---

## 一、实验目的

1. 理解虚拟地址、物理地址、页表、页表项和 TLB 的关系。
2. 理解 Sv39 三级页表如何通过 PDX1、PDX0 和 PTX 定位页表项。
3. 实现 page_insert，建立或更新线性地址到物理页的映射。
4. 实现 page_remove，撤销映射并维护物理页引用计数。
5. 理解页表项、引用计数和 TLB 必须保持一致的原因。
6. 分析 get_pte 中页表项查找和中间页表分配的设计取舍。

---

## 二、实验环境

- OS2026 已有 lab4 分支，本次没有新建分支。
- 目标架构：RISC-V 64。
- 实验目录：code/labcodes/lab4/lab4_new。
- 交叉编译器前缀：riscv64-unknown-elf-。
- 模拟器：QEMU 4.1.1。
- 成员 A：2411046 梁家瑞。

成员 A 的代码修改集中在：

    code/labcodes/lab4/lab4_new/kern/mm/pmm.c

修改位置保留 LAB4 function1 注释，并将 YOUR CODE 替换为 2411046。

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

Lab4 在 Lab3 的 Trap 机制上继续实现进程管理。整体分为三层：

1. 虚拟内存页操作：利用页表建立和撤销线性地址到物理页的映射；
2. 进程创建及初始化：用 PCB、内核栈、TrapFrame 和 Context 描述内核线程；
3. 进程调度与切换：选择可运行线程并保存、恢复 Context。

成员 A 负责第一层。页映射是后续进程创建的基础，页表根地址、内核栈和进程地址空间都依赖正确的页管理。如果引用计数错误，后续回收可能出现悬空页或重复释放；如果修改页表后不刷新 TLB，CPU 可能继续使用旧翻译。

### 3.2 功能实现顺序

    kern_init
      -> pmm_init
      -> page_init
      -> 建立 boot_pgdir
      -> check_pgdir / check_boot_pgdir
      -> proc_init
      -> cpu_idle
      -> schedule
      -> proc_run
      -> switch_to

页操作的调用关系是：

    线性地址 la
      -> get_pte(pgdir, la, create)
      -> 得到目标 PTE
      -> page_insert 或 page_remove
      -> 更新 PTE 和 Page.ref
      -> tlb_invalidate

get_pte 负责定位页表项，并按需创建中间页表；page_insert 和 page_remove 负责改变叶子 PTE 和物理页引用关系。

---

## 四、实验内容与实现

### 4.1 Sv39 页表与 get_pte

本工程使用 Sv39 风格的三级页表。有效虚拟地址由三个 9 位 VPN 和 12 位页内偏移组成：

    | VPN[2] 9 bit | VPN[1] 9 bit | VPN[0] 9 bit | offset 12 bit |
         PDX1           PDX0           PTX             PGOFF

页表页大小为 4096 字节，每个页表项为 8 字节，所以每页有 512 个页表项。

get_pte 的流程是：

1. 用 PDX1(la) 定位根页表中的一级项；
2. 若一级项无效且 create 为真，分配并清零下一级页表；
3. 用 PDX0(la) 定位第二级页表项；
4. 若第二级项无效且 create 为真，再分配并清零页表页；
5. 用 PTX(la) 定位最后一级 PTE 并返回；
6. 若 create 为假且中间页表不存在，返回 NULL，不分配页。

### 4.2 page_insert

函数作用是把物理页 page 映射到线性地址 la，并设置权限 perm。

实现步骤：

1. 用 get_pte(pgdir, la, 1) 获取目标 PTE；
2. 失败时返回 -E_NO_MEM；
3. 若目标已有有效映射，取出旧物理页；
4. 若旧页就是新页，只更新权限，不改变引用计数；
5. 若旧页不同，先调用 page_remove 删除旧映射；
6. 对新页执行 page_ref_inc；
7. 用 pte_create(page2ppn(page), perm) 写入新 PTE；
8. 调用 tlb_invalidate；
9. 返回 0。

同一物理页重新映射到同一地址时，不能再次增加引用计数，因为映射数量没有改变，只是权限可能发生变化。替换为另一物理页时，旧页引用减一，新页引用加一。

### 4.3 page_remove

函数作用是撤销线性地址 la 的有效映射。

实现步骤：

1. 用 get_pte(pgdir, la, 0) 查询 PTE；
2. PTE 不存在或无效时直接返回；
3. 用 pte2page 找到旧物理页；
4. 执行 page_ref_dec；
5. 将 PTE 清零；
6. 调用 tlb_invalidate。

page_remove 是幂等操作。对没有映射的地址调用不会分配新页，也不会报错；连续删除两次不会重复减少引用计数。

page_remove 不直接调用 free_page。解除一个映射不等于物理页生命周期结束，该页可能还被其他线性地址映射或由调用者持有。这里仅维护映射引用，最终释放由物理页所有者负责。

### 4.4 PTE、引用计数和 TLB 的一致性

- PTE 记录虚拟页到物理页的关系；
- Page.ref 记录物理页当前的引用数量；
- TLB 缓存虚拟页到物理页的硬件翻译。

建立映射时，PTE 指向新页、引用计数加一、旧 TLB 项失效。删除映射时，PTE 清零、引用计数减一、旧 TLB 项失效。只修改其中一个都会导致页表内容、软件计数和处理器实际翻译不一致。

### 4.5 Challenge 2：get_pte 两段代码为什么相似

Sv32、Sv39、Sv48 都采用多级页表。每一级都重复相同的抽象流程：

1. 从虚拟地址取出当前级 VPN；
2. 用 VPN 作为页表数组下标；
3. 检查页表项有效位；
4. 缺失且允许创建时分配下一级页表；
5. 从页表项中取出下一级页表物理地址；
6. 继续遍历。

所以 get_pte 中“一级到二级”和“二级到三级”的代码结构相似，只是层级和索引宏不同。Sv32 通常是两级，Sv39 是三级，Sv48 是四级；遍历次数不同，但“检查、创建、进入下一级”的逻辑不变。

当前工程的 Sv39 地址划分为 PDX1、PDX0、PTX 和 PGOFF，分别对应 VPN[2]、VPN[1]、VPN[0] 和页内偏移。

### 4.6 Challenge 2：查找和分配是否应拆分

当前 get_pte 通过 create 参数同时支持两种行为：

- create 为 0：只查找，不产生副作用；
- create 为 1：查找时按需创建中间页表。

优点是接口简单，page_insert 可以直接请求“确保 PTE 存在”，get_page 和 page_remove 可以请求“只查询”。缺点是查找和分配的副作用集中在一个函数中，调用者必须正确传递 create；如果中间层分配到一半失败，还要考虑已分配页表页的回收。

更复杂的内核可以拆成 find_pte 和 create_pte：前者只查找，后者负责分配。拆分后语义更清晰、测试更容易，但接口和代码更多。当前教学工程的 create 参数是简洁性和功能完整性之间的折中。

---

## 五、Spec、Prompt、迭代与问题分析

### 5.1 成员 A 的 Spec

Pre-Condition：

- 物理页分配器、struct Page、页表宏和 get_pte 已存在；
- pgdir 是有效页表根地址；
- page 是可用物理页；
- tlb_invalidate 可用。

Post-Condition：

- page_insert 成功时，la 的有效 PTE 指向 page，权限等于 perm，引用计数正确；
- page_remove 后 la 不再有有效映射，旧页引用计数减一；
- 未映射地址删除时不分配页表、不报错；
- 页表修改后旧地址翻译失效；
- 中间页表分配失败返回 -E_NO_MEM，不能留下半完成叶子映射。

测试 Case：

1. 空地址建立新映射；
2. 一个物理页映射到多个线性地址；
3. 目标地址从旧页改映射到新页；
4. 同一页同一地址只改变权限；
5. 删除有效映射；
6. 删除不存在的映射；
7. 中间页表分配失败；
8. 删除后再次插入并检查引用计数。

### 5.2 本次 AI 协作范围

AI 只用于阅读 report/report.md、实验要求和现有源码，梳理函数调用关系、检查报告章节结构和润色文字。核心代码依据当前工程中的 get_pte、check_pgdir、pmm.h 接口和测试约束完成核对，不将 AI 描述为生成核心实验代码。

本次报告整理提示词如下：

    [PROMPT]
    请阅读 OS2026 Lab4 的 report/report.md、lab4 实验要求、实验报告模板和成员 A 负责的 page_insert/page_remove/get_pte 内容，帮助我整理 lab4-A.md 的实验报告结构。重点解释页表层级、引用计数、TLB 失效、函数流程、Challenge 2、Spec 和验证边界。

    [RELY]
    只使用工程中的实际函数、宏、结构体和测试。成员 A 负责 pmm.c 的 page_insert、page_remove 和 Challenge 2；成员 B/C 的代码只作为依赖背景。

    [GUARANTEE]
    不生成或修改核心实验代码；不虚构 make、QEMU、grade 或截图结果；不把其他成员模块写成已通过；报告结论必须和源码及实际测试边界一致。

    [SPECIFICATION]
    输出符合实验报告模板的 Markdown 报告，包含实验目的、环境、整体逻辑、成员 A 的核心函数、实现不变量、Challenge 2、Spec、风险、OS 原理对应关系和测试占位。AI 只负责阅读、结构整理和文字润色。

### 5.3 实现风险与处理

风险 1：同一物理页重复映射导致引用计数增加。处理：检测旧 PTE 指向的页是否就是新页；相同则只更新权限。

风险 2：替换旧页时遗留旧映射。处理：先 page_remove，再写入新 PTE。

风险 3：查询函数意外创建页表。处理：page_remove 使用 get_pte 的 create=0。

风险 4：CPU 使用旧 TLB。处理：建立、替换和删除后都调用 tlb_invalidate。

风险 5：误以为解除映射就应立即释放物理页。处理：page_remove 只减少映射引用，不直接 free_page。

---

## 六、测试与验证

工程内 check_pgdir 和 tools/test_lab4.c 覆盖：

- 建立映射后能通过 get_pte 找到目标页；
- 一个物理页映射到多个线性地址；
- 用新页替换旧页；
- 同一地址同一页重新设置权限时引用计数不增加；
- 删除映射后引用计数恢复；
- 重复删除不会出错；
- PTE_U、PTE_W 权限正确写入；
- 页表更新后执行 TLB 失效。

建议在 Linux/WSL 中执行：

    cd /mnt/d/codex/操作系统/OS2026/code/labcodes/lab4/lab4_new
    make clean
    make GCCPREFIX=riscv64-unknown-elf-
    make grade

只观察启动时可执行：

    make qemu GCCPREFIX=riscv64-unknown-elf-

没有实际终端证据的结果不写成通过。成员 B/C 的进程创建和调度测试需要全组代码完成后再判断。

---

## 七、重要知识点与 OS 原理对应

| 实验知识点 | OS 原理含义 | 实验体现 |
|---|---|---|
| 虚拟内存 | 为程序提供独立、连续且受保护的地址空间抽象 | 线性地址通过页表映射到物理页 |
| 多级页表 | 用树形结构减少页表常驻开销 | Sv39 的 PDX1、PDX0、PTX |
| 页表项 | 记录物理页号、权限和有效位 | pte_create、PTE_V/PTE_U/PTE_W |
| 引用计数 | 记录物理页仍被多少映射使用 | page_ref_inc/page_ref_dec |
| TLB | 缓存虚拟页到物理页的翻译 | tlb_invalidate |
| 按需分配页表 | 使用虚拟区域时才创建中间页表 | get_pte 的 create 参数 |

本实验没有实现完整的缺页处理、页面置换、磁盘交换、Copy-on-Write、多核 TLB shootdown、NUMA 分配或完整用户空间隔离。

### OS 原理中重要但本实验未直接覆盖的知识点

1. 缺页异常和按需调页；
2. 页面置换算法和交换分区；
3. Copy-on-Write；
4. 多核 TLB shootdown；
5. 用户/内核地址空间隔离；
6. 大页、透明大页和页表压缩；
7. NUMA 内存分配；
8. 内存映射文件和共享内存。

---

## 八、实验总结

成员 A 的实现核心是维护一个不变量：每个有效叶子 PTE 都对应一个真实物理页，该物理页的引用计数包含这条映射；PTE 被替换或删除时，旧引用同步减少，新引用同步增加。get_pte 负责把线性地址转换为 PTE 地址，page_insert 负责建立关系，page_remove 负责撤销关系，tlb_invalidate 负责让处理器看到最新页表内容。

这说明虚拟内存不是单独的地址转换函数，而是页表结构、物理页生命周期、访问权限和硬件缓存共同组成的管理机制。后续进程创建和调度都建立在正确的页映射之上。
