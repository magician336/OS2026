# Lab2 Prompt 记录

本文件记录成员 A 的报告整理提示词和成员 C 的集成核验提示词。成员 B 的完整协作提示词记录保存在 `report/promptB.md`。代码生成、测试和报告整理的事实边界以实际提交和运行记录为准。

## Prompt C-1：集成与验证

```text
[PROMPT]
作为 lab2 成员 C，基于当前 OS2026/code/labcodes/lab2/lab2_new 代码，整合成员 A 的 page_init/Buddy System 与成员 B 的 Best-Fit/SLUB，检查 pmm_manager 接口、管理器选择、Makefile 接入和报告交付结构。

[RELY]
- 现有 pmm.c、pmm.h、default_pmm.c、best_fit_pmm.c、buddy_pmm.c、slub.c 和对应测试。
- 成员 A、B 已提供的阶段报告、提示词、截图和测试记录。
- 实验要求中的 make、make qemu、make grade 和 report 目录约束。

[GUARANTEE]
- 只把实际执行过的命令和真实输出写成验证结果。
- 工具链、QEMU 或环境缺失必须记录为环境限制，不得伪造通过。
- 不覆盖成员 A、B 的未提交修改，不提交 bin、obj、日志或本地工具数据库。

[SPECIFICATION]
- 核对 pmm_init() -> init_pmm_manager() -> page_init() -> init_memmap() -> alloc_pages()/free_pages() 的接口链。
- 在可用环境执行 make、make qemu、make grade，并记录失败原因和输出。
- 汇总 report/report.md、report/prompt.md、report/images/ 的交付状态。
```

## Prompt A-1：整理 page_init 与 First-Fit 报告结构

```text
[PROMPT]
**任务**：参考 code/labcodes/lab2/lab2_new/kern/mm/pmm.c、pmm.h、memlayout.h 和 default_pmm.c，修改并润色 report/lab2-A.md 中物理内存初始化与 First-Fit 的报告结构。
**操作要求**：只修改实验报告，不修改任何源代码；以工程中的真实定义为依据，按实验执行顺序组织内容。
**输出要求**：直接修改 report/lab2-A.md，保留报告模板的主要章节，不增加未经实际验证的结果。

[RELY]
- 真实接口：page_init、get_memory_base、get_memory_size、init_memmap、pa2page、default_init、default_init_memmap、default_alloc_pages、default_free_pages。
- 真实状态：va_pa_offset、npage、nbase、pages、PageReserved、PageProperty、property、page_link。
- 报告模板包含实验目的、环境、整体逻辑、功能实现、测试验证和总结。

[GUARANTEE]
必须修改和润色以下报告内容：
- page_init 模块说明
- First-Fit 四个核心函数说明
- First-Fit 改进空间
- 地址转换、Page 描述符和保留页知识点
- 未在实验中对应的 OS 原理知识点
不得修改 code/labcodes/lab2/lab2_new 下的任何文件，不得虚构测试输出、截图或 AI 生成代码的事实。

[SPECIFICATION]
## report/lab2-A.md 的 page_init 与 First-Fit 部分
**Pre-Condition**:
- 当前项目源码和实验报告模板均可读取。
- 报告结论必须能够由当前源码或实际测试支持。

**Post-Condition**:
- 报告按“整体逻辑 -> page_init -> First-Fit -> 知识点”的顺序组织。
- 每个函数说明与真实代码中的状态变化相符。
- 未实测内容保留为待验证说明，不写成已通过。

**Case 1**:
- 如果源码定义与草稿表述不一致，以源码为准修改报告。

**Case 2**:
- 如果草稿把推测写成测试结果，改为条件说明或待验证说明。

**Requirements**:
- 只修改、润色报告结构。
- 不生成实验代码，不在报告中加入截图命令，不添加无关内容。
```

## Prompt A-2：润色 Buddy System 实现说明结构

```text
[PROMPT]
**任务**：参考 code/labcodes/lab2/lab2_new/kern/mm/buddy_pmm.c、buddy_pmm.h、pmm.h 和 memlayout.h，修改并润色 report/lab2-A.md 中 Buddy System 实现说明的结构。
**操作要求**：只整理现有代码中的阶次空闲链表、初始区间分解、分裂、释放、伙伴定位和合并；不得修改源代码。
**输出要求**：直接修改 report/lab2-A.md，使 Buddy System 实现部分按数据结构和执行流程组织，不增加源码中不存在的功能。

[RELY]
- 真实接口：buddy_init、buddy_init_memmap、buddy_alloc_pages、buddy_free_pages、buddy_nr_free_pages、buddy_check、buddy_pmm_manager。
- 上层接口：struct pmm_manager、alloc_pages、free_pages、nr_free_pages。
- 页面状态：PageReserved、PageProperty、property、page_link。
- 报告中的截图位置位于 report/images，不提供或编造图片内容。

[GUARANTEE]
必须修改和润色以下报告内容：
- Buddy 数据结构
- 初始化区间分解
- 分配时的阶次计算与分裂
- 释放时的伙伴定位与合并
- 实现边界
不得修改 code/labcodes/lab2/lab2_new 下的任何文件，不得声称 AI 生成了代码，不得虚构编译、QEMU、grade 或截图结果。

[SPECIFICATION]
## report/lab2-A.md 的 Buddy System 实现部分
**Pre-Condition**:
- 已有可阅读的 Buddy System 实现、统一 pmm_manager 接口和实验报告模板。
- 所有行为说明必须与当前实现一致。

**Post-Condition**:
- 报告按“数据结构 -> 初始化 -> 分配 -> 释放合并”的顺序解释实现。
- 说明非二次幂请求、空间不足和伙伴越界时的行为。

**Case 1**:
- 请求不是二的幂时，说明向上取整和实际分配块大小的差异。

**Case 2**:
- 高阶块存在时，说明连续分裂以及右半块进入低阶链表的状态变化。

**Case 3**:
- 释放后伙伴可合并时，说明异或定位、边界检查和重复合并。

**Requirements**:
- 只修改、润色报告结构。
- 源代码、测试结果、截图和环境信息必须保持事实准确。
```

## Prompt A-3：润色 Buddy System 测试说明结构

```text
[PROMPT]
**任务**：参考 code/labcodes/lab2/lab2_new/kern/mm/buddy_pmm.c 中的 buddy_check，修改并润色 report/lab2-A.md 中 Buddy System 测试说明的结构。
**操作要求**：只整理现有测试覆盖的分裂、不同阶次、耗尽失败、伙伴合并、释放复用和计数恢复；不得修改代码，不得生成新测试。
**输出要求**：直接修改 report/lab2-A.md，使每项测试说明都对应真实断言；图片位置保持为空，不写入截图命令。

[RELY]
- buddy_check 使用独立的 16 页描述符区间。
- 3 页请求实际使用 4 页阶次块，另有 4 页和 8 页请求。
- 释放后检查 4+4 合并为 8、8+8 合并为 16，并重新分配 16 页。
- 实际测试结果只能来自终端输出；报告当前仅保留截图占位。

[GUARANTEE]
必须修改和润色以下报告内容：
- 测试前置状态
- 分裂与阶次断言
- 耗尽失败断言
- 两级伙伴合并断言
- 释放后复用断言
- 截图占位
不得修改任何 C 文件，不得虚构 QEMU 或 grade 结果，不得把截图命令写入报告。

[SPECIFICATION]
## report/lab2-A.md 的 Buddy System 测试部分
**Pre-Condition**:
- buddy_check 的当前源码可作为报告事实依据。
- 测试截图尚未添加。

**Post-Condition**:
- 报告按测试执行顺序说明输入、断言和预期状态。
- 每个“通过”结论都需要实际终端输出支持；当前仅描述覆盖目标。
- 三个图片链接保留为空位，正文不包含截图命令。

**Case 1**:
- 3、4、8 页分配耗尽 16 页测试区时，说明阶次向上取整和分裂结果。

**Case 2**:
- 耗尽后继续请求 1 页时，说明预期返回空指针。

**Case 3**:
- 按顺序释放两个 4 页块和一个 8 页块时，说明两级伙伴合并断言。

**Case 4**:
- 合并后重新请求 16 页时，说明释放复用和空闲计数恢复断言。

**Requirements**:
- 只修改、润色测试报告结构。
- 不修改代码，不写截图命令，不编造测试输出。
```
