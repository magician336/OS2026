/*
 * LAB4 function3 2411041
 * switch_to: 线程寄存器现场迁移,只切换 callee-saved 集合。
 * struct context 字段顺序为 ra, sp, s0..s11(proc.h),与下面的保存/恢复偏移一一对应。
 * 被换回的线程从 proc_run 内 switch_to 调用点继续执行,其 C 栈帧依赖的正是
 * callee-saved 寄存器跨调用不变的 ABI 约定;a0/a1 是 from/to 指针,无需保存。
 */
#include <riscv.h>

	.text
	.globl switch_to
switch_to:
	# a0 = from, a1 = to
	sd ra, 0*REGBYTES(a0)
	sd sp, 1*REGBYTES(a0)
	sd s0, 2*REGBYTES(a0)
	sd s1, 3*REGBYTES(a0)
	sd s2, 4*REGBYTES(a0)
	sd s3, 5*REGBYTES(a0)
	sd s4, 6*REGBYTES(a0)
	sd s5, 7*REGBYTES(a0)
	sd s6, 8*REGBYTES(a0)
	sd s7, 9*REGBYTES(a0)
	sd s8, 10*REGBYTES(a0)
	sd s9, 11*REGBYTES(a0)
	sd s10, 12*REGBYTES(a0)
	sd s11, 13*REGBYTES(a0)

	ld ra, 0*REGBYTES(a1)
	ld sp, 1*REGBYTES(a1)
	ld s0, 2*REGBYTES(a1)
	ld s1, 3*REGBYTES(a1)
	ld s2, 4*REGBYTES(a1)
	ld s3, 5*REGBYTES(a1)
	ld s4, 6*REGBYTES(a1)
	ld s5, 7*REGBYTES(a1)
	ld s6, 8*REGBYTES(a1)
	ld s7, 9*REGBYTES(a1)
	ld s8, 10*REGBYTES(a1)
	ld s9, 11*REGBYTES(a1)
	ld s10, 12*REGBYTES(a1)
	ld s11, 13*REGBYTES(a1)

	ret
