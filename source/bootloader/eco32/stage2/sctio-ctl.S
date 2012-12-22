#
# sctio-ctl.s -- disk sector I/O for disk made available by disk controller
#

# copied from the eco32 distribution

#***************************************************************

	.set	dskbase,0xF0400000	# disk base address
	.set	dskctrl,0			# control register
	.set	dskcnt,4			# count register
	.set	dsksct,8			# sector register
	.set	dskcap,12			# capacity register
	.set	dskbuf,0x00080000	# disk buffer

	.set	ctrlstrt,0x01		# start bit
	.set	ctrlien,0x02		# interrupt enable bit
	.set	ctrlwrt,0x04		# write bit
	.set	ctrlerr,0x08		# error bit
	.set	ctrldone,0x10		# done bit
	.set	ctrlrdy,0x20		# ready bit

	.set	sctsize,512			# sector size in bytes

	.set	retries,10000000	# retries to get disk ready

	.global	sctcapctl			# determine disk capacity
	.global	sctioctl			# do disk I/O

#***************************************************************

.section .text
.align	4

sctcapctl:
	add	$8,$0,retries			# set retry count
	add	$9,$0,dskbase
sctcap1:
	ldw	$10,$9,dskctrl
	and	$10,$10,ctrlrdy			# ready?
	bne	$10,$0,sctcapok			# yes - jump
	sub	$8,$8,1
	bne	$8,$0,sctcap1			# try again
	add	$2,$0,0					# no disk found
	j	sctcapx
sctcapok:
	ldw	$2,$9,dskcap			# get disk capacity
sctcapx:
	jr	$31

sctioctl:
	sub	$29,$29,24
	stw	$31,$29,20
	stw	$16,$29,16
	stw	$17,$29,12
	stw	$18,$29,8
	stw	$19,$29,4
	stw	$20,$29,0
	add	$17,$5,$0				# sector number
	or	$18,$6,0xC0000000		# memory address, virtualized
	add	$19,$7,$0				# number of sectors

sctrd:
	add	$2,$0,$0				# return ok
	beq	$19,$0,sctx				# if no (more) sectors
	add	$8,$0,dskbase
	add	$9,$0,1
	stw	$9,$8,dskcnt			# number of sectors
	stw	$17,$8,dsksct			# sector number on disk
	add	$9,$0,ctrlstrt
	stw	$9,$8,dskctrl			# start command
sctrd1:
	ldw	$2,$8,dskctrl
	and	$9,$2,ctrldone			# done?
	beq	$9,$0,sctrd1			# no - wait
	and	$9,$2,ctrlerr			# error?
	bne	$9,$0,sctx				# yes - leave
	add	$8,$0,dskbase + dskbuf	# transfer data
	add	$9,$0,sctsize
sctrd2:
	ldw	$10,$8,0				# from disk buffer
	stw	$10,$18,0				# to memory
	add	$8,$8,4
	add	$18,$18,4
	sub	$9,$9,4
	bne	$9,$0,sctrd2
	add	$17,$17,1				# increment sector number
	sub	$19,$19,1				# decrement number of sectors
	j	sctrd					# next sector

sctx:
	ldw	$20,$29,0
	ldw	$19,$29,4
	ldw	$18,$29,8
	ldw	$17,$29,12
	ldw	$16,$29,16
	ldw	$31,$29,20
	add	$29,$29,24
	jr	$31
