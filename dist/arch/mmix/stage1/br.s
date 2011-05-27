#
# br.s -- the boot record
#

	.set		SECTOR_COUNT,		64
	.set		SECTOR_SIZE,		512

.section .text

	.global	start

	# reset arrives here
start:
	GETA		$0,stack
	UNSAVE	0,$0

	SETL		$1,0
	GETA		$2,strtmsg
	PUSHJ		$0,puts

	SETL		$0,1							# current sec
	SETML		$1,#0010
	ORH			$1,#8000					# destination = 1MiB
	SETL		$2,SECTOR_SIZE
1:
	CMPU		$3,$0,SECTOR_COUNT
	BZ			$3,2f
	OR			$4,$0,$0
	OR			$5,$1,$1
	PUSHJ		$3,readSec				# readSec(sec,dst)
	ADDU		$0,$0,1						# to next sector
	ADDU		$1,$1,$2
	JMP			1b

2:
	SETML		$0,#0010
	ORH			$0,#8000
	GO			$0,$0,0						# go to bootloader

# void readSec(octa sec,octa *buf)
readSec:
	GET		$2,rJ								# save rJ
	SETH	$3,#8003						# disk-address
	STCO	1,$3,#8							# sector-count = 1
	STOU	$0,$3,#16						# sector-number = sec
	STCO	#1,$3,#0						# start read-command
	# wait until the DONE-bit is set
1:
	LDOU	$4,$3,#0
	AND		$4,$4,#10
	PBZ		$4,1b
	# now read one sector from the disk-buffer into buf
	SET		$4,0
	SET		$5,512
	INCML	$3,#0008						# address of disk-buffer
2:
	LDOU	$6,$3,$4
	STOU	$6,$1,$4
	ADDU	$4,$4,8							# next octa
	CMPU	$6,$4,$5						# all 512 bytes copied?
	PBNZ	$6,2b
	PUT		rJ,$2								# restore rJ
	POP		0,0

# void puts(octa term,char *string)
puts:
	GET		$2,rJ								# save rJ
1:
	LDBU	$5,$1,0							# load char from string
	BZ		$5,2f								# if its 0, we are done
	ADDU	$4,$0,0
	PUSHJ	$3,putc							# call putc(c)
	ADDU	$1,$1,1							# to next char
	JMP		1b
2:
	PUT		rJ,$2								# restore rJ
	POP		0,0

# void putc(octa term,octa character)
putc:
	SETH	$2,#8002						# base address: #8002000000000000
	SL		$0,$0,32						# or in terminal-number
	OR		$2,$2,$0						# -> #8002000100000000 for term 1, e.g.
1:
	LDOU	$3,$2,#10						# read ctrl-reg
	AND		$3,$3,#1						# exract RDY-bit
	PBZ		$3,1b								# wait until its set
	STOU	$1,$2,#18						# write char
	POP		0,0

	# messages
	.align 4
strtmsg:
	.asciz	"BR executing\r\n"
	.align 4
dremsg:
	.asciz	"dsk read err\r\n"
	.align 4
hltmsg:
	.asciz	"bootstrap halted\r\n"

	# stack for unsave
	OCTA	#0								% rL
	OCTA	#0								% $255 = ff
	OCTA	#0								% rB
	OCTA	#0								% rD
	OCTA	#0								% rE
	OCTA	#0								% rH
	OCTA	#0								% rJ
	OCTA	#0								% rM
	OCTA	#0								% rP
	OCTA	#0								% rR
	OCTA	#0								% rW
	OCTA	#0								% rX
	OCTA	#0								% rY
	OCTA	#0								% rZ
stack:
	OCTA	#FF00000000000000	% rG | rA

