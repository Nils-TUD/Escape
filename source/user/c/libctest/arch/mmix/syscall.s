/**
 * $Id$
 */

.section	.text

.global		doSyscall7
.global		doSyscall

# int doSyscall7(uint syscallNo,uint arg1,uint arg2,uint arg3,uint arg4,uint arg5,uint arg6,uint arg7)
doSyscall7:
	SET		$9,$0
	SET		$0,$1
	SET		$1,$2
	SET		$2,$3
	SET		$3,$4
	SET		$4,$5
	SET		$5,$6
	SET		$6,$7
	SET		$7,$8
	SETH	$8,#0000
	ORML	$8,#0000
	SLU		$9,$9,8
	OR		$8,$8,$9					# TRAP 0,$9,0
	PUT		rX,$8
	GETA	$8,@+12
	PUT		rW,$8
	RESUME	0
	BZ		$2,1f						# no-error?
	SET		$0,$2
1:
	POP		1,0

# int doSyscall(uint syscallNo,uint arg1,uint arg2,uint arg3)
doSyscall:
	SET		$9,$0
	SET		$0,$1
	SET		$1,$2
	SET		$2,$3
	SETH	$8,#0000
	ORML	$8,#0000
	SLU		$9,$9,8
	OR		$8,$8,$9					# TRAP 0,$9,0
	PUT		rX,$8
	GETA	$8,@+12
	PUT		rW,$8
	RESUME	0
	BZ		$2,1f						# no-error?
	SET		$0,$2
1:
	POP		1,0
