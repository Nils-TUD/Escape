/**
 * $Id$
 */

.section	.text

.global		doSyscall7
.global		doSyscall

# int doSyscall7(uint syscallNo,uint arg1,uint arg2,uint arg3,uint arg4,uint arg5,uint arg6,uint arg7)
doSyscall7:
	add		$2,$0,$4
	add		$4,$0,$5
	add		$5,$0,$6
	add		$6,$0,$7
	ldw		$7,$29,16
	ldw		$8,$29,20
	ldw		$9,$29,24
	ldw		$10,$29,28
	trap
	beq		$11,$0,1f
	add		$2,$11,$0
1:
	jr		$31

# int doSyscall(uint syscallNo,uint arg1,uint arg2,uint arg3)
doSyscall:
	add		$2,$0,$4
	add		$4,$0,$5
	add		$5,$0,$6
	add		$6,$0,$7
	trap
	beq		$11,$0,1f
	add		$2,$11,$0
1:
	jr		$31
