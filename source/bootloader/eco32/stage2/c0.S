/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

	.set	TERM_BASE,0xF0300000			# terminal base address
	.set	OUTPUT_BASE,0xFF000000			# output base address
	.set	TIMER_BASE,0xF0000000			# timer base address
	.set	TIMER_REG_CTRL,0
	.set	TIMER_REG_DIV,4
	.set	TIMER_CTRL_EXP,0x1
	.set	TIMER_CTRL_IEN,0x2
	.set	PAGE_SIZE,0x1000
	.set	FS_PSW,0
	.set	FS_TLB_INDEX,1
	.set	FS_TLB_EHIGH,2
	.set	FS_TLB_ELOW,3
	.set	FS_TSC_HIGH,5
	.set	FS_TSC_LOW,6
	.set	CINTRPT_FLAG,1 << 23			# flag for current-interrupt-enable
	.set	INTRPTS_IN_RAM,1 << 27			# the flag to handle interrupts in RAM (not ROM)


	.extern	bootload

	.extern	debugString
	.extern	sctcapctl
	.extern	sctioctl

	.global reset
	.global dskio
	.global debugChar


.section .text

reset:
	j 		start

interrupt:
	j		handleIntrpt

userMiss:
	j		handleUserMiss

handleIntrpt:
	add		$4,$0,errIntrpt
	jal		debugString
	j		loop

handleUserMiss:
	add		$4,$0,errUserMiss
	jal		debugString
	j		loop

loop:
	j		loop

start:
	add		$19,$4,$0						# save $4
	# setup psw
	mvfs	$8,FS_PSW
	or		$8,$8,INTRPTS_IN_RAM			# let vector point to RAM
	and		$8,$8,~CINTRPT_FLAG				# disable interrupts (just to be sure)
	mvts	$8,FS_PSW

	# initialize the TLB with invalid entries
	add		$16,$0,0xC0000000
	add		$17,$0,$0						# loop index
	add		$18,$0,32						# loop end
initTLBLoop:
	mvts	$17,FS_TLB_INDEX				# set index
	mvts	$16,FS_TLB_EHIGH				# set entryHi
	mvts	$0,FS_TLB_ELOW					# set entryLo
	tbwi									# write TLB entry at index
	add		$17,$17,1						# $17++
	add		$16,$16,PAGE_SIZE
	blt		$17,$18,initTLBLoop				# if $17 < $18, jump to initTLBLoop

	# determine CPU speed
	add		$8,$0,TIMER_BASE
	add		$9,$0,100						# 100ms measurement
	stw		$9,$8,TIMER_REG_DIV
	add		$9,$0,TIMER_CTRL_IEN
	stw		$9,$8,TIMER_REG_CTRL			# enable timer interrupts
	mvfs	$9,FS_PSW
	or		$9,$9,CINTRPT_FLAG				# enable interrupts
	or 		$9,$9,0xFFFF					# set interrupt mask
	mvts	$9,FS_PSW
	add		$0,$0,$0						# nop; we might jump over that
	add		$18,$0,0
	# get TSC (low is enough)
	mvfs	$9,FS_TSC_LOW
	# wait for interrupt
1:
	beq		$18,$0,1b
	# get TSC
	mvfs	$10,FS_TSC_LOW
	# speed = (after - before) * 10
	sub		$16,$10,$9
	mulu	$16,$16,10						# store CPU speed in callee save reg
	mvfs	$9,FS_PSW
	and		$9,$9,~CINTRPT_FLAG				# disable interrupts
	and		$9,$9,~0xFFFF
	mvts	$9,FS_PSW

	# load modules
	add		$4,$19,$0						# give bootload the memory size
	add		$29,$0,0xC0200000 - 4			# setup stack
	jal		bootload						# call bootload function

	add		$5,$0,$16						# give the kernel the CPU speed
	add		$4,$2,$0						# give the kernel the boot infos
	add		$31,$0,0xC0000000				# jump to kernel
	jr		$31

# void debugChar(char c)
debugChar:
	sub		$29,$29,8						# create stack frame
	stw		$31,$29,0						# save return register
	stw		$16,$29,4						# save register variable
	add		$16,$4,0						# save argument
	add		$10,$0,0x0A
	bne		$4,$10,printCharStart
	add		$4,$0,0x0D
	jal		debugChar
printCharStart:
	add		$8,$0,TERM_BASE					# set I/O base address
	add		$10,$0,OUTPUT_BASE				# set output base address
printCharLoop:
	ldw		$9,$8,(0 << 4 | 8)				# get xmtr status
	and		$9,$9,1							# xmtr ready?
	beq		$9,$0,printCharLoop				# no - wait
	stw		$16,$10,0						# send char to output
	stw		$16,$8,(0 << 4 | 12)			# send char
	ldw		$31,$29,0						# restore return register
	ldw		$16,$29,4						# restore register variable
	add		$29,$29,8						# release stack frame
	jr		$31								# return

# int dskio(int dskno, char cmd, int sct, Word addr, int nscts)
dskio:
	add		$4,$5,$0
	add		$5,$6,$0
	add		$6,$7,$0
	ldw		$7,$29,16
	j		sctioctl

.section .data

errIntrpt:
	.asciz	"unexpected interrupt\r\n"
errUserMiss:
	.asciz	"unexpected user-miss\r\n"
