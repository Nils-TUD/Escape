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

#include <sys/arch.h>
#include <sys/asm.h>
#include <sys/syscalls.h>

.section .text

.extern main
.extern exit
.extern __libc_preinit
.extern _init
.extern initenv

.extern __preinit_array_start
.extern __preinit_array_end
.extern __init_array_start
.extern __init_array_end

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
BEGIN_FUNC(_start)
	// mark the beginning of the call-trace
	mov		$0,%REG(bp)
	pop		%ARG_1
	mov		WORDSIZE * 0(%REG(sp)),%ARG_2
	mov		WORDSIZE * 1(%REG(sp)),%ARG_3

	// call __libc_preinit(entryPoint,argc,argv)
	call	__libc_preinit

	// it returns the entrypoint; 0 if we're the initial thread
	test	%REG(ax),%REG(ax)
	je		1f

	// we're an additional thread, so call the desired function
	mov		%REG(ax),%REG(dx)
	// get argument
	pop		%ARG_1
	// align it properly (SSE)
	sub		$8,%REG(sp)
	call	*%REG(dx)
	jmp		2f

	// initial thread calls main
1:
	// init env stuff
	mov		WORDSIZE * 2(%REG(sp)),%ARG_1
	mov		WORDSIZE * 3(%REG(sp)),%ARG_2
	call	initenv

	// call init functions
	call	call_init_array

	// get argc and argv
	pop		%ARG_1
	pop		%ARG_2
	// finally, call main
	call	main

2:
	mov		%REG(ax),%ARG_1
	call	exit
	// just to be sure
	1: jmp	1b
END_FUNC(_start)

.macro acall array
	mov		$\array\()_start,%REG(ax)
	// are we at the end of the array?
1:	mov		$\array\()_end,%REG(cx)
	cmp		%REG(ax),%REG(cx)
	je		2f
	// load function
	mov		(%REG(ax)),%REG(dx)
	// save *ax and call function
	push	%REG(ax)
	call	*%REG(dx)
	pop		%REG(ax)
	// to next function
	add		$WORDSIZE,%REG(ax)
	jmp		1b
2:
.endm

// calls the constructors in .init_array and .ctors sections
BEGIN_FUNC(call_init_array)
	push	%REG(bp)
	mov		%REG(sp),%REG(bp)

	acall	__preinit_array
	call	_init
	acall	__init_array

	leave
	ret
END_FUNC(call_init_array)

// all signal-handler return to this "function"
BEGIN_FUNC(sigRetFunc)
	// we have to use int instead of sysenter here because sysexit does not restore ecx,edx and eflags.
	// so, if we have started the signal during an interrupt/exception and were thus not prepared for
	// loosing one of those, we would get into trouble.
	mov		$ASM_SYSC_ACKSIG,%SYSCNO
	int		$ASM_IRQ_ACKSIG
	// never reached
END_FUNC(sigRetFunc)
#endif
