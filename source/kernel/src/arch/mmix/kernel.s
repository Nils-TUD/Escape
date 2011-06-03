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

	.extern	main

	.extern	_ecode
	.extern	_edata
	.extern	_ebss

	.global	_bcode
	.global	_bdata
	.global	_bbss

	.extern	dbg_prints
	.extern intrpt_handler
	.extern curPDir
	.extern util_panic

	.global cpu_getBadAddr

	.global start
	.global tlb_get
	.global tlb_set
	.global tlb_remove

	.global logByte
	.global debugc

	.global intrpt_setMask

	.global thread_idle
	.global	thread_save
	.global thread_resume

#===========================================
# Constants
#===========================================

	.set		PAGE_SIZE,0x1000
	.set		PAGE_SIZE_SHIFT,12
	.set		PTE_MAP_ADDR,0x80000000
	.set		DIR_MAP_START,0xC0000000
	.set		KERNEL_HEAP,0x80800000
	.set		KERNEL_STACK,0x84400FFC					% our kernel-stack (the top)
																					% individually for each process and stored fix in the
																					% first entry of the TLB

.section .text
_bcode:

#===========================================
# Start
#===========================================

start:
	# call main
	PUSHJ		$0,main													# call 'main' function

	# just to be sure
loop:
	JMP		loop

#===========================================
# Data
#===========================================

.section .data
_bdata:

.section .bss
_bbss:
