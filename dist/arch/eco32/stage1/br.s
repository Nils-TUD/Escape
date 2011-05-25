#
# br.s -- the boot record
#

# Runtime environment:
#
# This code must be loaded and started at 0xC0000000.
# It allocates a stack from 0xC0001000 downwards. So
# it must run within 4K (code + data + stack).
#
# This code expects the disk number of the boot disk
# in $16, the start sector of the disk or partition
# to be booted in $17 and its size in $18.
#
# The bootstrap loader program, which is loaded
# by this code, must be in standalone (headerless)
# executable format, stored at partition relative
# disk sectors 1..7 (i.e. the boot block), and
# gets loaded and started at 0xC00F0000.

.section .text

	.set		TERM_BASE,0xF0300000		# terminal base address
	.set		OUTPUT_BASE,0xFF000000	# output base address
	.set		stacktop,0xC0001000	# top of stack
	.set		loadaddr,0xC00F0000	# where to load the boot loader

	.global	reset

	# reset arrives here
reset:
	add	$29,$0,stacktop		# setup stack
	add	$4,$0,strtmsg		# say what is going on
	jal	msgout
	add	$4,$0,1			# start loading with sector 1
	add	$5,$0,loadaddr		# where to load the boot loader
	and	$5,$5,0x3FFFFFFF	# convert to physical address
	add	$6,$0,7			# 7 sectors to load
	jal	rdsct
	add	$8,$0,loadaddr		# start executing the boot loader
	jr	$8

	# read disk sectors
	#   $4 start sector number (disk or partition relative)
	#   $5 transfer address
	#   $6 number of sectors
rdsct:
	sub	$29,$29,32
	stw	$31,$29,20
	add	$7,$0,16		# sector count
	add	$6,$5,$0		# transfer address
	add	$5,$4,$17		# relative sector -> absolute
	add	$4,$0,'r'		# command
	jal	sctioctl
	bne	$2,$0,rderr		# error?
	ldw	$31,$29,20
	add	$29,$29,32
	jr	$31

	# disk read error
rderr:
	add	$4,$0,dremsg
	jal	msgout
	j	halt

	# output message
	#   $4 pointer to string
msgout:
	sub	$29,$29,8
	stw	$31,$29,4
	stw	$16,$29,0
	add	$16,$4,0		# $16: pointer to string
msgout1:
	ldbu	$4,$16,0		# get character
	beq	$4,$0,msgout2		# done?
	jal	chrout			# output character
	add	$16,$16,1		# bump pointer
	j	msgout1			# continue
msgout2:
	ldw	$16,$29,0
	ldw	$31,$29,4
	add	$29,$29,8
	jr	$31

	# output character
	#   $4 character
chrout:
	add		$8,$0,TERM_BASE										# set I/O base address
2:
	ldw		$9,$8,(0 << 4 | 8)								# get xmtr status
	and		$9,$9,1														# xmtr ready?
	beq		$9,$0,2b													# no - wait
	stw		$4,$8,(0 << 4 | 12)							# send char
	jr		$31																# return

	# halt execution by looping
halt:
	add	$4,$0,hltmsg
	jal	msgout
halt1:
	j	halt1

	# messages
strtmsg:
	.asciz	"BR executing\r\n"
dremsg:
	.asciz	"dsk read err\r\n"
hltmsg:
	.asciz	"bootstrap halted\r\n"
