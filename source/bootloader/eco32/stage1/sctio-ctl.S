//
// sctio-ctl.s -- disk sector I/O for disk made available by disk controller
//

// copied from the eco32 distribution and adjusted

//***************************************************************

	.set	dskbase,0xF0400000	// disk base address
	.set	dskctrl,0			// control register
	.set	dskcnt,4			// count register
	.set	dsksct,8			// sector register
	.set	dskcap,12			// capacity register
	.set	dskbuf,0x00080000	// disk buffer

	.set	ctrlstrt,0x01		// start bit
	.set	ctrlien,0x02		// interrupt enable bit
	.set	ctrlwrt,0x04		// write bit
	.set	ctrlerr,0x08		// error bit
	.set	ctrldone,0x10		// done bit
	.set	ctrlrdy,0x20		// ready bit

	.set	sctsize,512			// sector size in bytes

	.global	sctioctl			// do disk I/O

//***************************************************************

.section .text
.align	4

// sctioctl(int sector,void *dest,int secCount)
sctioctl:
	add	$6,$6,0xC0000000		// memory address, virtualized
sctrd:
	add	$2,$0,$0				// return ok
	beq	$7,$0,sctx				// if no (more) sectors
	add	$8,$0,dskbase
	add	$9,$0,1
	stw	$9,$8,dskcnt			// number of sectors
	stw	$5,$8,dsksct			// sector number on disk
	add	$9,$0,ctrlstrt
	stw	$9,$8,dskctrl			// start command
sctrd1:
	ldw	$2,$8,dskctrl
	and	$9,$2,ctrldone			// done?
	beq	$9,$0,sctrd1			// no - wait
	and	$9,$2,ctrlerr			// error?
	bne	$9,$0,sctx				// yes - leave
	add	$8,$0,dskbase + dskbuf	// transfer data
	add	$9,$0,sctsize
sctrd2:
	ldw	$10,$8,0				// from disk buffer
	stw	$10,$6,0				// to memory
	add	$8,$8,4
	add	$6,$6,4
	sub	$9,$9,4
	bne	$9,$0,sctrd2
	add	$5,$5,1					// increment sector number
	sub	$7,$7,1					// decrement number of sectors
	j	sctrd					// next sector
sctx:
	jr	$31
