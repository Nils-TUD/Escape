//
// br.s -- the boot record
//

	.set	ELF_EH_SEC_OFF,				40
	.set	ELF_EH_SEC_SIZE,			58
	.set	ELF_SH_ADDR,				16
	.set	ELF_SH_OFF,					24
	.set	ELF_SH_SIZE,				32
	.set	ELF_SECTION_TEXT,			1
	.set	ELF_SECTION_REGS,			2

	.set	SECTOR_COUNT,				64
	.set	SECTOR_SIZE,				512
	.set	BUFFER,						#8000000000001000

.section .text

	.global	start

	// reset arrives here
start:
	PUT		rG,224
	// read first sector into buffer; it contains the elf-header
	SETH	$1,(BUFFER>>48)&0xFFFF
	ORMH	$1,(BUFFER>>32)&0xFFFF
	ORML	$1,(BUFFER>>16)&0xFFFF
	ORL		$1,(BUFFER>>0)&0xFFFF
	SETL	$3,1
	OR		$4,$1,$1
	SETL	$5,1
	PUSHJ	$2,readSecs					// readSecs(1,BUFFER,1)

	// init a few global vars
	SETL	$224,SECTOR_SIZE
	LDWU	$225,$1,ELF_EH_SEC_SIZE
	LDOU	$226,$1,ELF_EH_SEC_OFF

	// read section-header sectors into buffer
	DIVU	$3,$226,$224
	ADDU	$5,$3,1						// first sector is mbr
	OR		$6,$1,$1
	SETL	$7,2						// read 2 sectors just to be sure that all headers are incl.
	PUSHJ	$4,readSecs					// readSecs($5,BUFFER,2)

	// add remaining offset to buffer
	GET		$2,rR
	ADDU	$227,$1,$2
	// offset of text-section
	MULU	$1,$225,ELF_SECTION_TEXT
	ADDU	$1,$1,$227
	LDOU	$2,$1,ELF_SH_OFF
	DIVU	$2,$2,$224					// start-sector
	ADDU	$2,$2,1						// first sector is mbr
	LDOU	$5,$1,ELF_SH_ADDR
	OR		$228,$5,$5					// store start-addr for later usage
	GET		$4,rR
	SUBU	$3,$5,$4					// start a bit before the load-addr, so that the offset in
										// file gets loaded to the load-addr
	LDOU	$6,$1,ELF_SH_SIZE
	ADDU	$0,$5,$6					// store end-addr for later usage
	ADDU	$4,$6,$4					// add offset, because we start at -offset
	ADDU	$4,$4,$224
	SUBU	$4,$4,1
	DIVU	$4,$4,$224					// round up and to sectors
	PUSHJ	$1,readSecs

	OR		$2,$228,$228
	OR		$3,$0,$0
	SUBU	$3,$3,$2
	PUSHJ	$1,flushRegion				// flush the text-region

	// offset of reg-section
	MULU	$1,$225,ELF_SECTION_REGS
	ADDU	$1,$1,$227
	LDOU	$2,$1,ELF_SH_OFF
	DIVU	$2,$2,$224					// start-sector
	ADDU	$2,$2,1						// first sector is mbr
	2ADDU	$0,$224,$0
	SUBU	$0,$0,1						// end-addr + 512*2 - 1
	NEGU	$3,0,$224
	AND		$0,$0,$3					// end-addr & -512
	ADDU	$0,$0,8						// one slot for rL
	OR		$3,$0,$0
	GET		$4,rR
	SUBU	$3,$3,$4					// start a bit before the load-addr
	LDOU	$4,$1,ELF_SH_SIZE
	OR		$229,$4,$4					// store for later usage
	ADDU	$4,$4,$224
	SUBU	$4,$4,1
	DIVU	$4,$4,$224					// round up and to sectors
	PUSHJ	$1,readSecs

	// finally, we have to put rG|rA behind the reg-section
	SUBU	$0,$0,8						// one slot backwards
	STCO	0,$0,0						// set rL to 0
	DIVU	$2,$229,8
	SETL	$3,255
	SUBU	$2,$3,$2					// number of regs = 255 - size / 8
	SLU		$2,$2,56
	ADDU	$0,$0,$229					// go to end
	STCO	0,$0,8						// start at offset 8, because of rL
	STCO	0,$0,16
	STCO	0,$0,24
	STCO	0,$0,32
	STCO	0,$0,40
	STCO	0,$0,48
	STCO	0,$0,56
	STCO	0,$0,64
	STCO	0,$0,72
	STCO	0,$0,80
	STCO	0,$0,88
	STCO	0,$0,96
	ADDU	$0,$0,14*8					// 12 special registers + rL + rG|rA
	STOU	$2,$0,0

	// we're done, go to bootloader; address of registers is passed in $0
	GO		$228,$228,0

// void flushRegion(octa addr,octa count)
flushRegion:
	SETL	$2,#100
2:
	BNP		$1,1f						// count <= 0?
	SYNCD	#FF,$0,0					// flush to memory
	SYNCID	#FF,$0,0					// remove from caches
	SUB		$1,$1,$2
	ADDU	$0,$0,$2
	JMP		2b
1:
	POP		0,0

// void readSecs(octa sec,octa *buf,octa count)
readSecs:
	GET		$3,rJ
	SETH	$4,#8003					// disk-address
3:
	STCO	1,$4,#8						// sector-count = 1
	STOU	$0,$4,#16					// sector-number = sec
	STCO	#1,$4,#0					// start read-command
	// wait until the DONE-bit is set
1:
	LDOU	$5,$4,#0
	AND		$5,$5,#10
	PBZ		$5,1b
	// now read one sector from the disk-buffer into buf
	SET		$5,0
	SET		$6,512
	OR		$8,$4,$4
	INCML	$8,#0008					// address of disk-buffer
2:
	LDOU	$7,$8,$5
	STOU	$7,$1,$5
	ADDU	$5,$5,8						// next octa
	CMPU	$7,$5,$6					// all 512 bytes copied?
	PBNZ	$7,2b
	SUBU	$2,$2,1						// count--
	ADDU	$1,$1,$224					// to next sector in the buffer
	ADDU	$0,$0,1						// next sector
	PBNZ	$2,3b						// not finished?
	PUT		rJ,$3
	POP		0,0

