#
# c0.s -- startup code and begin-of-segment labels
#

	.extern	bootload
	.extern stack

	.global	_bcode
	.global	_bdata
	.global	_bbss

	.extern	debugString

	.global start
	.global sctcapctl
	.global dskio
	.global debugChar


.section .text
_bcode:

start:
	GETA		$0,stack
	UNSAVE	0,$0

	PUSHJ		$0,bootload				# call bootload function

	SETH		$1,#8000
	GO			$1,$1,0						# go to kernel; load-progs-array is in $0

# void debugChar(octa character)
debugChar:
	GET		$1,rJ
	SETH	$2,#8002						# base address: #8002000000000000
	CMPU	$3,$1,'\n'					# char = \n?
	BNZ		$3,1f
	SET		$4,'\r'
	PUSHJ	$3,debugChar				# putc('\r')
1:
	LDOU	$3,$2,#10						# read ctrl-reg
	AND		$3,$3,#1						# exract RDY-bit
	PBZ		$3,1b								# wait until its set
	STOU	$1,$2,#18						# write char
	PUT		rJ,$1
	POP		0,0

# int sctcapctl(void)
sctcapctl:
	# wait until ready-bit is set
	SETH	$0,#8003
1:
	LDOU	$1,$0,0
	AND		$1,$1,#20
	PBZ		$1,1B
	LDOU	$0,$0,24						# read capacity
	POP		0,0

# void dskio(int sct,void *addr,int nscts)
dskio:
	GET		$3,rJ								# save rJ
	SETH	$4,#8003						# disk-address
	STOU	$2,$4,#8						# sector-count = nscts
	STOU	$0,$4,#16						# sector-number = sct
	STCO	#1,$4,#0						# start read-command
	# wait until the DONE-bit is set
1:
	LDOU	$5,$4,#0
	AND		$5,$5,#10
	PBZ		$5,1b
	# now read the sectors from the disk-buffer into buf
	SET		$5,0
	SETL	$6,512							# sector size
	MULU	$6,$6,$2						# *= number of sectors
	INCML	$4,#0008						# address of disk-buffer
2:
	LDOU	$7,$4,$5
	STOU	$7,$1,$5
	ADDU	$5,$5,8							# next octa
	CMPU	$7,$5,$6						# ready?
	PBNZ	$7,2b
	PUT		rJ,$3								# restore rJ
	POP		0,0

.section .data
_bdata:

.section .bss
_bbss:
