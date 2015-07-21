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

.section .text

#include <sys/arch.h>
#include <sys/asm.h>
#include <sys/syscalls.h>

.extern __libc_preinit
.extern __libc_init
.extern initenv
.extern load_initHeap
.extern load_setupProg

//  Initial stack:
//  +------------------+  <- top
//  |       errno      |
//  +------------------+
//  |        TLS       | (pointer to the actual TLS)
//  +------------------+
//  |     arguments    |
//  |        ...       |
//  +------------------+
//  |       envv       |  not present for threads
//  +------------------+
//  |       envc       |  not present for threads
//  +------------------+
//  |       argv       |  not present for threads
//  +------------------+
//  |       argc       |  not present for threads
//  +------------------+
//  |    entryPoint    |  0 for initial thread, thread-entrypoint for others
//  +------------------+
//  |    fd for prog   |
//  +------------------+

BEGIN_FUNC(_start)
	// init heap here first
	call	load_initHeap

	// call __libc_preinit
	pop		%REG(bx)
	pop		%ARG_1
	mov		WORDSIZE * 0(%REG(sp)),%ARG_2
	mov		WORDSIZE * 1(%REG(sp)),%ARG_3
	call	__libc_preinit

	// push entrypoint again
	push	%REG(ax)

	// init env stuff
	mov		WORDSIZE * 3(%REG(sp)),%ARG_1
	mov		WORDSIZE * 4(%REG(sp)),%ARG_2
	call	initenv

	// now call __libc_init
	call	__libc_init

	// pass file-descriptor to load_setupProg
	mov		%REG(bx),%ARG_1
	mov		WORDSIZE * 1(%REG(sp)),%ARG_2
	mov		WORDSIZE * 2(%REG(sp)),%ARG_3
	call	load_setupProg

	// load_setupProg returns the entrypoint
	jmp		*%REG(ax)
END_FUNC(_start)

// all signal-handler return to this "function"
BEGIN_FUNC(sigRetFunc)
	mov		$ASM_SYSC_ACKSIG,%SYSCNO
	int		$ASM_IRQ_ACKSIG
	// never reached
END_FUNC(sigRetFunc)
