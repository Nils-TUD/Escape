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

	.set		TERM_BASE,0xF0300000						# terminal base address
	.set		OUTPUT_BASE,0xFF000000					# output base address
	.set		PAGE_SIZE,0x1000
	.set		FS_PSW,0
	.set		INTRPTS_IN_RAM,1 << 27					# the flag to handle interrupts in RAM (not ROM)
	.set		stacktop,0xC0001000							# top of stack
	.set		loadaddr,0xC0400000							# where to load the boot loader

	.global	reset

	# reset arrives here
reset:
	j			start

interrupt:
	j			isr

userMiss:
	j			halt

start:
	# setup psw
	mvfs	$8,FS_PSW
	or		$8,$8,INTRPTS_IN_RAM							# let vector point to RAM
	mvts	$8,FS_PSW

	# read boot loader
	add		$29,$0,stacktop										# setup stack
	add		$4,$0,1														# start loading with sector 1
	add		$5,$0,loadaddr										# where to load the boot loader
	and		$5,$5,0x3FFFFFFF									# convert to physical address
	add		$6,$0,31													# 31 sectors to load
	jal		rdsct

	# determine amount of memory
	add		$16,$0,0xC0000000
	add		$18,$0,$0													# will be set to 1 by the interrupt-handler
memLoop:
	ldw		$17,$16,0													# access page
	bne		$18,$0,memDone										# exception seen?
	add		$16,$16,PAGE_SIZE
	j			memLoop
memDone:
	sub		$4,$16,0xC0000000									# pass memory-amount to bootload

	# jump to boot loader
	add		$8,$0,loadaddr										# start executing the boot loader
	jr		$8

isr:
	add		$18,$0,1													# set flag
	add		$30,$30,4													# skip ldw
	rfx

	# read disk sectors
	#   $4 start sector number (disk or partition relative)
	#   $5 transfer address
	#   $6 number of sectors
rdsct:
	sub		$29,$29,32
	stw		$31,$29,20
	add		$7,$6,$0													# sector count
	add		$6,$5,$0													# transfer address
	add		$5,$4,$17													# relative sector -> absolute
	add		$4,$0,'r'													# command
	jal		sctioctl
	bne		$2,$0,rderr												# error?
	ldw		$31,$29,20
	add		$29,$29,32
	jr		$31

	# disk read error
rderr:
	add		$4,$0,dremsg
	jal		msgout
	j			halt

	# output message
	#   $4 pointer to string
msgout:
	sub		$29,$29,8
	stw		$31,$29,4
	stw		$16,$29,0
	add		$16,$4,0													# $16: pointer to string
msgout1:
	ldbu	$4,$16,0													# get character
	beq		$4,$0,msgout2											# done?
	jal		chrout														# output character
	add		$16,$16,1													# bump pointer
	j			msgout1														# continue
msgout2:
	ldw		$16,$29,0
	ldw		$31,$29,4
	add		$29,$29,8
	jr		$31

	# output character
	#   $4 character
chrout:
	add		$8,$0,TERM_BASE										# set I/O base address
2:
	ldw		$9,$8,(0 << 4 | 8)								# get xmtr status
	and		$9,$9,1														# xmtr ready?
	beq		$9,$0,2b													# no - wait
	stw		$4,$8,(0 << 4 | 12)								# send char
	jr		$31																# return

	# halt execution by looping
halt:
	add		$4,$0,hltmsg
	jal		msgout
halt1:
	j			halt1

	# messages
dremsg:
	.asciz	"disk read error\r\n"
hltmsg:
	.asciz	"bootstrap halted\r\n"
