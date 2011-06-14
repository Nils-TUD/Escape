/**
 * $Id: kernel.s 901 2011-06-02 20:25:32Z nasmussen $
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#===========================================
# Imports / Exports
#===========================================

	.global	_bcode
	.global	_bdata
	.global	_bbss

	.extern	main
	.extern intrpt_forcedTrap
	.extern intrpt_dynTrap

	.global start

	.global logByte
	.global debugc
	.global paging_setrV
	.global tc_update
	.global cpu_rdtsc
	.global cpu_getGlobal
	.global cpu_getSpecial

	.global thread_idle
	.global	thread_save
	.global thread_resume

#===========================================
# Constants
#===========================================

	.set	PAGE_SIZE,0x2000
	.set	PAGE_SIZE_SHIFT,13
	.set	STACK_SIZE,PAGE_SIZE
	.set	PTE_MAP_ADDR,0x80000000
	.set	DIR_MAP_START,0xC0000000
	.set	KERNEL_HEAP,0x80800000
	.set	KERNEL_STACK,0x84400FFC			% our kernel-stack (the top)

.section .text
_bcode:

#===========================================
# Start
#===========================================

start:
	# establish the register-stack
	UNSAVE	0,$0

	# store rG for kernel; we'll have to set it in the trap-handlers (the user may change it)
	GETA	$1,krg
	GET		$2,rG
	STOU	$2,$1,0

	# bootinfo is in $0
	LDOU	$254,$0,32						# load kstackbegin from bootinfo
	SETL	$1,STACK_SIZE
	2ADDU	$254,$1,$254					# += 2 * STACK_SIZE
	SUBU	$254,$254,8						# stack-pointer = kstackbegin + 2 * STACK_SIZE - 8
	OR		$253,$254,$254					# frame-pointer = stack-pointer

	# call main
	OR		$1,$0,$0						# pass bootinfo to main
	SET		$2,$254							# pass pointer to stackBegin, which is set in main
	SUBU	$3,$254,8						# pass pointer to rss, which is set in main
	PUSHJ	$0,main							# call 'main' function

	# setup forced and dynamic trap handlers
	GETA	$1,forcedTrap
	PUT		rT,$1
	GETA	$1,dynamicTrap
	PUT		rTT,$1

	# go to user mode (initloader)
	PUT		rWW,$0							# main returned the entry-point
	SETH	$0,#8000
	PUT		rXX,$0							# skip to rWW
	SUBU	$0,$254,8
	LDOU	$0,$0,0
	PUT		rSS,$0							# set kernel-stack
	LDOU	$0,$254,0						# pointer to stackBegin
	UNSAVE	0,$0							# setup initial stack for initloader
	NEG		$255,0,1						# set rK = -1
	RESUME	1

	# just to be sure
loop:
	JMP		loop

#===========================================
# Traps
#===========================================

forcedTrap:
	SAVE	$255,1
	# set rG
	GETA	$0,krg
	LDOU	$0,$0,0
	PUT		rG,$0
	# setup sp and fp
	GET		$0,rSS
	INCL	$0,STACK_SIZE
	SUBU	$254,$0,8
	SET		$253,$254
	SET		$0,$255							# save $255
	GET		$2,rSS
	PUSHJ	$1,intrpt_forcedTrap
	UNSAVE	1,$0
	NEG		$255,0,1						# rK = -1
	RESUME	1

dynamicTrap:
	SAVE	$255,1
	# set rG
	GETA	$0,krg
	LDOU	$0,$0,0
	PUT		rG,$0
	# setup sp and fp
	GET		$0,rSS
	INCL	$0,STACK_SIZE
	SUBU	$254,$0,8
	SET		$253,$254
	# put various information into a struct on the stack and pass it to the handler
	SUBU	$254,$254,6*8					# determine irq-number
	GET		$0,rQ
	SUBU	$1,$0,1
	SADD	$3,$1,$0
	GET		$2,rS
	SET		$0,$255							# save $255
	# call handler
	PUSHJ	$1,intrpt_dynTrap				# call intrpt_dynTrap(rS,irqNo)
	UNSAVE	1,$0
	PUT		rQ,0							# TODO temporary!
	NEG		$255,0,1						# rK = -1
	RESUME	1

#===========================================
# Paging
#===========================================

# void paging_setrV(uint64_t rv)
paging_setrV:
	PUT		rV,$0
	POP		0,0

#===========================================
# TC
#===========================================

# void tc_update(uint64_t key)
tc_update:
	LDVTS	$0,$0,0
	POP		0,0

#===========================================
# CPU
#===========================================

# uint64_t cpu_rdtsc(void)
cpu_rdtsc:
	GET		$0,rC
	POP		1,0

# uint64_t cpu_getGlobal(int rno)
cpu_getGlobal:
	SETH	$1,#0000
	ORML	$1,#C100
	SLU		$0,$0,8
	OR		$1,$1,$0						# ORI $0,$<rno>,0
	PUT		rX,$1
	GETA	$0,@+12
	PUT		rW,$0
	RESUME	0
	POP		1,0

# uint64_t cpu_getSpecial(int rno)
cpu_getSpecial:
	SETH	$1,#0000
	ORML	$1,#FE00
	OR		$1,$1,$0						# GET $0,<rno>
	PUT		rX,$1
	GETA	$0,@+12
	PUT		rW,$0
	RESUME	0
	POP		1,0

#===========================================
# Input/Output
#===========================================

# void debugc(char character)
debugc:
	GET		$1,rJ
	SETH	$2,#8002						# base address: #8002000000000000
	CMPU	$3,$0,0xA						# char = \n?
	BNZ		$3,1f
	SET		$4,0xD
	PUSHJ	$3,debugc						# putc('\r')
1:
	LDOU	$3,$2,#10						# read ctrl-reg
	AND		$3,$3,#1						# exract RDY-bit
	PBZ		$3,1b							# wait until its set
	STOU	$0,$2,#18						# write char
	PUT		rJ,$1
	POP		0,0

krg:
	OCTA	0

#===========================================
# Data
#===========================================

.section .data
_bdata:

.section .bss
_bbss:
