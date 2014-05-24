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

#include <esc/syscalls.h>
#include <esc/arch.h>

.section .text

.global _start
.global sigRetFunc

_start:
	// load modules first
	mov		$ASM_SYSC_LOADMODS,%rdi
	syscall

	// load vm86-task
	mov		$ASM_SYSC_VM86START,%rdi
	syscall

	// now replace with init
	mov		$progName,%rsi					// arg1: path
	mov		$args,%r10						// arg2: arguments
	mov		$0,%r8							// arg3: env
	mov		$ASM_SYSC_EXEC,%rdi				// set syscall-number
	syscall

	// we should not reach this
1:	jmp		1b

// provide just a dummy
sigRetFunc:
	jmp		sigRetFunc

.section .data

args:
	.quad	progName,0

progName:
	.asciz	"/bin/init"
