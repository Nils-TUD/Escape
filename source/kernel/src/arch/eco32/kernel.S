/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//===========================================
// Imports / Exports
//===========================================

	.extern	bspstart
	.extern unittesT_start

	.extern	dbg_prints
	.extern intrpt_handler
	.extern curPDir
	.extern Util::panic

	.global cpu_getBadAddr

	.global start
	.global tlb_remove

	.global logByte
	.global debugc

	.global intrpt_setMask

	.global thread_startup
	.global thread_idle
	.global	thread_save
	.global thread_resume

//===========================================
// Constants
//===========================================

	.set	TERM_BASE,0xF0300000			// terminal base address
	.set	OUTPUT_BASE,0xFF000000			// output base address
	.set	TIMER_BASE,0xF0000000			// timer base address
	.set	KEYBOARD_BASE,0xF0200000		// keyboard base address

	.set	PAGE_SIZE,0x1000
	.set	PAGE_BITS,12
	.set	PTE_MAP_ADDR,0x80000000
	.set	DIR_MAP_START,0xC0000000
	.set	KERNEL_HEAP,0x80400000
	.set	KERNEL_STACK,0x84000FFC			// our kernel-stack (the top)
											// individually for each process and stored fix in the
											// first entry of the TLB
	.set	TLB_INDEX_INVALID,0x80000000	// represents an invalid index

	// special-registers
	.set	FS_PSW,0
	.set	FS_TLB_INDEX,1
	.set	FS_TLB_EHIGH,2
	.set	FS_TLB_ELOW,3
	.set	FS_TLB_BAD,4

	// psw
	.set	CINTRPT_FLAG,1 << 23			// flag for current-interrupt-enable
	.set	PINTRPT_FLAG,1 << 22			// previous
	.set	OINTRPT_FLAG,1 << 21			// old
	.set	CUSER_MODE_FLAG,1 << 26			// current user-mode
	.set	PUSER_MODE_FLAG,1 << 25			// previous user-mode
	.set	OUSER_MODE_FLAG,1 << 24			// old user-mode
	.set	INTRPTS_IN_RAM,1 << 27			// the flag to handle interrupts in RAM (not ROM)

//===========================================
// Entry-points & TLB-handler
//===========================================

.section .text

reset:
	j		start

interrupt:
	j		isr

// user TLB misses arrive here (for the addresses 0x00000000 - 0x7FFFFFFF)
intrpt_userMiss:
.nosyn
	// note that we can use $27 and $26 here since it is not used in isr
	// we have to store $30 because ldw may cause a tlb-miss
	add		$26,$30,0
	mvfs	$27,FS_TLB_EHIGH				// load tlbEntryHi
	slr		$27,$27,PAGE_BITS - 2			// determine "global" page offset
	// NOTE that this assumes that the lower 16 bits of PTE_MAP_ADDR are 0
	ldhi	$28,PTE_MAP_ADDR				// build address in mapped area
	add		$27,$27,$28
	ldw		$27,$27,0						// load page-table-entry
	mvts	$27,FS_TLB_ELOW					// use this for entryLow
	tbwr									// write into TLB
	// restore $30
	add		$30,$26,0
	rfx
.syn

//===========================================
// Start
//===========================================

start:
	// setup psw
	mvfs	$8,FS_PSW
	or		$8,$8,INTRPTS_IN_RAM			// let vector point to RAM
	and		$8,$8,~CINTRPT_FLAG				// disable interrupts (just to be sure)
	mvts	$8,FS_PSW

	// TLB is already cleared here by the bootloader

	// setup stack
	add		$29,$0,_ebss					// setup the initial stack directly behind the bss-segment
	and		$29,$29,~(PAGE_SIZE - 1)		// because the bootloader has reserved at least 1 and max.
	add		$29,$29,PAGE_SIZE * 2 - 4		// 2 pages - 1 word space for us

	// enable all interrupts via mask (we'll control them just by the interrupt-enable-flag)
	add		$16,$4,$0						// save $4
	add		$17,$5,$0						// save $5
	add		$4,$0,0xFFFF
	jal		intrpt_setMask

	// enable keyboard-interrupts to be able to react on F12 for entering the debugging-console before
	// the keyboard device is running
	add		$8,$0,KEYBOARD_BASE
	add		$9,$0,0x2
	stw		$9,$8,0

	// call bspstart
	add		$4,$16,$0						// give bspstart the load-progs-array
	add		$5,$17,$0						// give bspstart the CPU speed
	add		$6,$29,$0						// pointer to user stack-pointer
	jal		bspstart
	add		$16,$2,$0						// save entry-point
	// if we've started the unittest-kernel, we'll register the tests here
	jal		unittest_start

	// initloader has been loaded, so jump to it
	ldw		$29,$29,0						// load user stack-pointer
	add		$30,$0,$16						// smpstart returns the entry-point
	mvfs	$8,FS_PSW
	// enable interrupts and user-mode
	or		$8,$8,PINTRPT_FLAG | PUSER_MODE_FLAG
	mvts	$8,FS_PSW
	rfx

	// just to be sure
loop:
	j		loop

//===========================================
// Interrupts
//===========================================

isr:
	// interrupts are already disabled here, setup kernel-stack
	.nosyn
	mvfs	$28,FS_PSW						// load psw
	slr		$28,$28,25						// extract prev usermode
	and		$28,$28,1
	beq		$28,$0,isrStackUsed				// 1 => new stack
	ldhi	$28,KERNEL_STACK				// load higher 16 bit
	or		$28,$28,KERNEL_STACK			// or lower 16 bit
	j		isrStackPresent
isrStackUsed:
	ldhi	$28,KERNEL_STACK				// load higher 16 bit
	bgeu	$29,$28,isrStackOk				// valid if $29 >= 0x80000000
	or		$28,$28,KERNEL_STACK			// or lower 16 bit
	j		isrStackPresent
isrStackOk:
	add		$28,$29,0						// use the current stack-pointer

isrStackPresent:
	// now save registers to kernel-stack
	sub		$28,$28,34 * 4
	stw		$9,$28,36						// first store $9 and $10 to have 2 registers
	stw		$10,$28,40
	mvfs	$9,FS_PSW
	slr		$9,$9,16
	and		$9,$9,0x1F						// determine irq-number
	add		$10,$0,21						// kernel-miss
	beq		$9,$10,intrpt_exKMiss			// go directly to kernel-miss handler, if it is one
	// no kernel-miss -> ordinary procedure
	stw		$1,$28,4
	.syn
	stw		$2,$28,8
	stw		$3,$28,12
	stw		$4,$28,16
	stw		$5,$28,20
	stw		$6,$28,24
	stw		$7,$28,28
	stw		$8,$28,32
	// $9 and $10 are saved
	stw		$11,$28,44
	stw		$12,$28,48
	stw		$13,$28,52
	stw		$14,$28,56
	stw		$15,$28,60
	stw		$16,$28,64
	stw		$17,$28,68
	stw		$18,$28,72
	stw		$19,$28,76
	stw		$20,$28,80
	stw		$21,$28,84
	stw		$22,$28,88
	stw		$23,$28,92
	stw		$24,$28,96
	stw		$25,$28,100
	stw		$29,$28,116
	stw		$30,$28,120
	stw		$31,$28,124

	// get interrupt-number
	mvfs	$4,FS_PSW
	stw		$4,$28,128						// store psw
	slr		$4,$4,16
	and		$4,$4,0x1F
	stw		$4,$28,132						// store irq-number
	// setup stack-pointer and handle interrupt
	sub		$29,$28,4
	add		$4,$28,0						//	the intrpt-handler wants to access the registers
	jal		intrpt_handler

	// rebuild $28. maybe we got a tlb-miss
	add		$28,$29,4

	// restore psw
	ldw		$8,$28,128
	mvts	$8,FS_PSW

	// restore register
	ldw		$2,$28,8
	ldw		$3,$28,12
	ldw		$4,$28,16
	ldw		$5,$28,20
	ldw		$6,$28,24
	ldw		$7,$28,28
	ldw		$8,$28,32
	ldw		$9,$28,36
	ldw		$10,$28,40
	ldw		$11,$28,44
	ldw		$12,$28,48
	ldw		$13,$28,52
	ldw		$14,$28,56
	ldw		$15,$28,60
	ldw		$16,$28,64
	ldw		$17,$28,68
	ldw		$18,$28,72
	ldw		$19,$28,76
	ldw		$20,$28,80
	ldw		$21,$28,84
	ldw		$22,$28,88
	ldw		$23,$28,92
	ldw		$24,$28,96
	ldw		$25,$28,100
	ldw		$29,$28,116
	ldw		$30,$28,120
	ldw		$31,$28,124
	.nosyn
	ldw		$1,$28,4
	rfx										// return from exception
	.syn

// kernel-TLB-miss-handler (for the addresses 0x80000000 - 0xBFFFFFFF)
intrpt_exKMiss:
.nosyn
	// we may only use $9 and $10 here!
	ldhi	$9,curPDir						// put page-dir address into $9
	or		$9,$9,curPDir
	ldw		$9,$9,0							// page-dir is already in dir-mapped-space
	mvfs	$10,FS_TLB_EHIGH				// load tlbEntryHi
	slr		$10,$10,20						// grab the offset of the page-table
	and		$10,$10,0xFFC
	add		$9,$9,$10						// add to page-directory
	ldw		$9,$9,0							// load direct mapped page-table-address
	beq		$9,$0,intrpt_exKMissWrite		// does the page-table exist?
	ldhi	$10,0xFFFFFFFF
	or		$10,$10,0xFFF8
	and		$9,$9,$10						// remove flags
	ldhi	$10,0xC0000000
	or		$9,$9,$10						// make directly addressable
	mvfs	$10,FS_TLB_EHIGH				// load tlbEntryHi
	slr		$10,$10,10						// grab the page-number
	and		$10,$10,0xFFC
	add		$9,$9,$10						// add to page-table
	ldw		$9,$9,0							// load page-table-entry
intrpt_exKMissWrite:
	mvts	$9,FS_TLB_ELOW					// page-table-entry to tlbEntryLo
	tbwr									// write virt. & phys. address into TLB
	// restore tlb-Entry-High for the case that we have got a TLB-miss
	ldhi	$9,PTE_MAP_ADDR
	sub		$10,$27,$9						// revert to entryHi
	sll		$10,$10,PAGE_BITS - 2
	mvts	$10,FS_TLB_EHIGH				// store in TLB
	ldw		$9,$28,36						// restore $9 and $10
	ldw		$10,$28,40
	rfx
.syn

// int intrpt_andMask(int mask)
intrpt_setMask:
	mvfs	$8,FS_PSW
	and		$2,$8,0x0000FFFF 				// store old mask as return value
	and		$4,$4,0x0000FFFF				// use lower 16 bits only
	or		$8,$8,$4
	mvts	$8,FS_PSW
	jr		$31

//===========================================
// Input/Output
//===========================================

// void debugc(char c)
debugc:
	sub		$29,$29,8						// create stack frame
	stw		$31,$29,0						// save return register
	stw		$16,$29,4						// save register variable
	add		$16,$4,0						// save argument
	add		$10,$0,0x0A
	bne		$4,$10,debugcStart
	add		$4,$0,0x0D
	jal		debugc
debugcStart:
	add		$8,$0,TERM_BASE					// set I/O base address
	add		$10,$0,OUTPUT_BASE				// set output base address
debugcLoop:
	ldw		$9,$8,(0 << 4 | 8)				// get xmtr status
	and		$9,$9,1							// xmtr ready?
	beq		$9,$0,debugcLoop				// no - wait
	stw		$16,$10,0						// send char to output
	stw		$16,$8,(0 << 4 | 12)			// send char
	ldw		$31,$29,0						// restore return register
	ldw		$16,$29,4						// restore register variable
	add		$29,$29,8						// release stack frame
	jr		$31								// return

//===========================================
// TLB
//===========================================

// void tlb_remove(unsigned int addr)
tlb_remove:
	and		$4,$4,~(PAGE_SIZE - 1)			// get the page-start
	mvts	$4,FS_TLB_EHIGH					// search for the virtual address
	tbs
	mvfs	$8,FS_TLB_INDEX					// now we have the index
	add		$9,$0,TLB_INDEX_INVALID
	beq		$8,$9,tlb_removeExit			// TLB_INDEX_INVALID = entry not found
	add		$8,$0,DIR_MAP_START
	mvts	$8,FS_TLB_EHIGH					// use this as entry-high
	mvts	$0,FS_TLB_ELOW					// invalidate
	tbwi									// write index
tlb_removeExit:
	jr		$31


//===========================================
// Threads
//===========================================

thread_startup:
	// $16 = process entry-point
	// $17 = thread entry-point
	// $18 = argument
	add		$8,$0,DIR_MAP_START
	bltu	$17,$8,2f
	// stay in kernel
	jalr	$17
1:
	j		1b

2:
	// setup user-env
	sub		$29,$29,0x20
	add		$4,$0,$18
	add		$5,$0,$17
	jal		uenv_setupThread		// call uenv_setupThread(arg,entryPoint)

	// go to user-mode
	add		$29,$0,$2				// user-stackpointer
	add		$30,$0,$16				// start here
	mvfs	$8,FS_PSW
	// enable interrupts and user-mode
	or		$8,$8,PINTRPT_FLAG | PUSER_MODE_FLAG
	mvts	$8,FS_PSW
	rfx

// void thread_idle()
thread_idle:
	// enable interrupts
	mvfs	$8,FS_PSW
	or		$8,$8,CINTRPT_FLAG
	mvts	$8,FS_PSW
	// loop until an interrupt arrives
thread_idleLoop:
	j		thread_idleLoop

// save a tasks context
// bool thread_save(ThreadRegs *saveArea)
thread_save:
	// save callee-save registers
	stw		$16,$4,0
	stw		$17,$4,4
	stw		$18,$4,8
	stw		$19,$4,12
	stw		$20,$4,16
	stw		$21,$4,20
	stw		$22,$4,24
	stw		$23,$4,28
	stw		$25,$4,32
	stw		$29,$4,36
	stw		$30,$4,40
	stw		$31,$4,44
	add		$2,$0,0							// return 0
	jr		$31

// resume a task
// bool thread_resume(pagedir_t pageDir,ThreadRegs *saveArea,frameno_t kstackFrame);
thread_resume:
	// save parameter (we can use $16 since the current process
	// will not continue after resume)
	add		$16,$5,$0

	// first load the new stack-pointer; accessing the saveArea may cause a kernel-miss
	// therefore it has to be done before the page-dir has been exchanged and the new kernel-stack
	// has been set
	ldw		$29,$16,36

	// set page-directory for new process
	stw		$4,$0,curPDir

	// we have to refresh the fix entry for the process
	mvts	$0,FS_TLB_INDEX
	ldhi	$10,KERNEL_STACK				// ASSUMES that only the high 16 bit are interesting here
	mvts	$10,FS_TLB_EHIGH
	sll		$8,$6,PAGE_BITS					// frame-number to address
	or		$8,$8,0x7						// make existing, present and writable
	mvts	$8,FS_TLB_ELOW
	tbwi

	// remove all entries from the TLB that come from not-shared pagetables
	add		$9,$0,KERNEL_HEAP
	add		$4,$0,1							// loop index (first index is fix)
	add		$5,$0,DIR_MAP_START				// 0xC0000000
	add		$6,$0,$0						// entryLo (stays always the same)
	add		$7,$0,32						// loop end
resumeClearTLBLoop:
	mvts	$4,FS_TLB_INDEX
	tbri
	mvfs	$8,FS_TLB_EHIGH
	bltu	$8,$9,1f						// virt < KERNEL_HEAP?
	bltu	$8,$10,2f						// virt < KERNEL_STACK?
1:
	mvts	$5,FS_TLB_EHIGH					// not in the shared-page-table-area
	mvts	$6,FS_TLB_ELOW					// therefore, invalidate the entry
	tbwi
2:
	add		$4,$4,1							// $4++
	blt		$4,$7,resumeClearTLBLoop		// if $4 < $7, jump to resumeClearTLBLoop

	// restore registers
	ldw		$17,$16,4
	ldw		$18,$16,8
	ldw		$19,$16,12
	ldw		$20,$16,16
	ldw		$21,$16,20
	ldw		$22,$16,24
	ldw		$23,$16,28
	ldw		$25,$16,32
	ldw		$30,$16,40
	ldw		$31,$16,44
	ldw		$16,$16,0

	add		$2,$0,1							// return 1
	jr		$31
