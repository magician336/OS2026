#ifndef __KERN_MM_SLUB_H__
#define __KERN_MM_SLUB_H__

#include <defs.h>

// 单核教学版本：16～2048字节cache，加上连续多页的大对象分配。
// 任意正字节数请求受可用连续物理页限制；默认按16字节对齐。
// 零大小、算术溢出或空间不足返回NULL；无效或重复释放触发panic。
#define SLUB_DEFAULT_ALIGN 16

void slub_init(void);
void *kmalloc(size_t size);
// alignment为1～PGSIZE之间的2的幂；小于16时按16处理。
void *kmalloc_aligned(size_t size, size_t alignment);
void kfree(void *ptr);
// 启动阶段调用，测试前不能有其他SLUB活跃对象。
void slub_check(void);

#endif
