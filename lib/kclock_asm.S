#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
#include <kclock.h>



.macro	setup_c0_status set clr
	.set	push			// 存所有的设置
	mfc0	t0, CP0_STATUS	// 状态寄存器，cp0-12号
	or	t0, \set|\clr		// 把set和clr或进t0
	xor	t0, \clr			// 把clr异或进t0
	mtc0	t0, CP0_STATUS	// 把新的t0给CP0_STATUS		
	.set	pop
.endm

	.text
LEAF(set_timer)

	li t0, 0x01
	sb t0, 0xb5000100
	// 0xb5000000 是模拟器(gxemul) 映射实时钟的位置
	// 偏移量为0x100 表示来设置实时钟中断的频率,1 则表示1 秒钟中断1次，如果写入0，表示关闭实时钟
	sw	sp, KERNEL_SP
setup_c0_status STATUS_CU0|0x1001 0
	jr ra

	nop
END(set_timer)

