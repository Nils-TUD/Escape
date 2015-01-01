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

	.extern	bootload
	.extern stack

	.extern	debugString

	.global start
	.global debugChar
	.global flushRegion

	.set	NONEX_MEM_EX,				#4
	.set	PAGE_SIZE,					#2000
	.set	STACK_SIZE,					PAGE_SIZE

.section .text

start:
	// setup global and special registers; the MBR has setup $0 for us
	UNSAVE	0,$0

	// setup software-stack
	GET		$0,rR						// end of BSS is in rR
	SET		$1,STACK_SIZE
	2ADDU	$0,$1,$0
	NEGU	$2,0,$1
	AND		$254,$0,$2					// setup stack-pointer
	OR		$253,$254,$254				// setup frame-pointer

	// determine the amount of main memory
	SETH	$0,#8000
	SET		$2,PAGE_SIZE
1:
	LDOU	$1,$0,0
	GET		$1,rQ
	BNZ		$1,2f
	ADDU	$0,$0,$2
	JMP		1b
2:
	// unset non-existing-memory-exception
	PUT		rQ,0
	SETH	$1,#8000
	SUBU	$1,$0,$1
	PUSHJ	$0,bootload					// call bootload function

	LDOU	$0,$0,0						// get bootinfo struct
	SETH	$1,#8000
	GO		$1,$1,0						// go to kernel

// void debugChar(octa character)
debugChar:
	GET		$1,rJ
	SETH	$2,#8002					// base address: //8002000000000000
	CMPU	$3,$0,0xA					// char = \n?
	BNZ		$3,1f
	SET		$4,0xD
	PUSHJ	$3,debugChar				// putc('\r')
1:
	LDOU	$3,$2,#10					// read ctrl-reg
	AND		$3,$3,#1					// exract RDY-bit
	PBZ		$3,1b						// wait until its set
	STOU	$0,$2,#18					// write char
	PUT		rJ,$1
	POP		0,0

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
