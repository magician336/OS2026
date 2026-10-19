#ifndef __KERN_MM_SLUB_H__
#define __KERN_MM_SLUB_H__

#include <defs.h>

// 单核教学版本：1～64 字节请求使用 64 字节槽位，返回地址按 16 字节对齐。
// 零大小、超过 64 字节或内存不足返回 NULL；无效或重复释放触发 panic。
void slub_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void slub_check(void);
#endif
