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

#include <sys/syscalls.h>
#include <sys/arch.h>

.section .text

.global _start
.global sigRetFunc
.extern main
.extern exit
.extern __libc_preinit
.extern initenv
.extern _init

//  Initial stack:
//  +------------------+  <- top
//  |       errno      |
//  +------------------+
//  |        TLS       | (pointer to the actual TLS)
//  +------------------+
//  |     arguments    |
//  |        ...       |  not present for threads
//  +------------------+
//  |       envv       |  not present for threads
//  +------------------+
//  |       envc       |  not present for threads
//  +------------------+
//  |       argv       |  not present for threads
//  +------------------+
//  |       argc       |  or the argument for threads
//  +------------------+
//  |    entryPoint    |  0 for initial thread, thread-entrypoint for others
//  +------------------+  <- 16 byte aligned (+8)

#ifndef SHAREDLIB
_start:
	// mark the beginning of the call-trace
	mov		$0,%REG(bp)
	pop		%ARG_1
	mov		WORDSIZE * 0(%REG(sp)),%ARG_2
	mov		WORDSIZE * 1(%REG(sp)),%ARG_3

	// call __libc_preinit(entryPoint,argc,argv)
	call	__libc_preinit

	// it returns the entrypoint; 0 if we're the initial thread
	test	%REG(ax),%REG(ax)
	je		initialThread

	// we're an additional thread, so call the desired function
	mov		%REG(ax),%REG(dx)
	// get argument
	pop		%ARG_1
	// align it properly (SSE)
	sub		$8,%REG(sp)
	call	*%REG(dx)
	jmp		threadExit

	// initial thread calls main
initialThread:
	// init env stuff
	mov		WORDSIZE * 2(%REG(sp)),%ARG_1
	mov		WORDSIZE * 3(%REG(sp)),%ARG_2
	call	initenv

	// call function in .init-section
	call	_init

	// get argc and argv
	pop		%ARG_1
	pop		%ARG_2
	// finally, call main
	call	main

threadExit:
	mov		%REG(ax),%ARG_1
	call	exit
	// just to be sure
	1: jmp	1b

// all signal-handler return to this "function"
sigRetFunc:
	// we have to use int instead of sysenter here because sysexit does not restore ecx,edx and eflags.
	// so, if we have started the signal during an interrupt/exception and were thus not prepared for
	// loosing one of those, we would get into trouble.
	mov		$ASM_SYSC_ACKSIG,%SYSCNO
	int		$ASM_IRQ_ACKSIG
	// never reached
#endif
