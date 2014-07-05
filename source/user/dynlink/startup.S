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

#include <esc/syscalls.h>
#include <esc/arch.h>

.global sigRetFunc
.global _start
.extern __libc_preinit
.extern __libc_init
.extern initenv
.extern load_initHeap
.extern load_setupProg

//  Initial stack:
//  +------------------+  <- top
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
//  |     TLSSize      |  0 if not present
//  +------------------+
//  |     TLSStart     |  0 if not present
//  +------------------+
//  |    entryPoint    |  0 for initial thread, thread-entrypoint for others
//  +------------------+
//  |    fd for prog   |
//  +------------------+

_start:
	// init heap here first
	call	load_initHeap

	// call __libc_preinit
	pop		%REG(bx)
	pop		%ARG_1
	pop		%ARG_2
	pop		%ARG_3
#if defined(__x86_64__)
	mov		WORDSIZE * 0(%REG(sp)),%ARG_4
	mov		WORDSIZE * 1(%REG(sp)),%ARG_5
#endif
	call	__libc_preinit

	// since we do not support TLS with dynamic linking yet, just pass arbitrary values as TLS-stuff
	push	$0
	push	$0
	// push entrypoint again
	push	%REG(ax)

	// init env stuff
	mov		WORDSIZE * 5(%REG(sp)),%ARG_1
	mov		WORDSIZE * 6(%REG(sp)),%ARG_2
	call	initenv

	// now call __libc_init
	call	__libc_init

	// pass file-descriptor to load_setupProg
	mov		%REG(bx),%ARG_1
	// pass pointers to TLSSize and TLSStart to load_setupProg
	lea		WORDSIZE * 1(%REG(sp)),%ARG_2
	lea		WORDSIZE * 2(%REG(sp)),%ARG_3
#if defined(__x86_64__)
	mov		WORDSIZE * 3(%REG(sp)),%ARG_4
	mov		WORDSIZE * 4(%REG(sp)),%ARG_5
#endif
	call	load_setupProg

	// load_setupProg returns the entrypoint
	jmp		*%REG(ax)

// all signal-handler return to this "function"
sigRetFunc:
	mov		$ASM_SYSC_ACKSIG,%SYSCNO
	int		$ASM_IRQ_ACKSIG
	// never reached
