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
//  |     TLSSize      |  0 if not present
//  +------------------+
//  |     TLSStart     |  0 if not present
//  +------------------+
//  |    entryPoint    |  0 for initial thread, thread-entrypoint for others
//  +------------------+

#ifndef SHAREDLIB
_start:
	// mark the beginning of the call-trace
	mov		$0,%ebp
	pop		%eax
	pop		%edx
	pop		%ecx
	// call __libc_preinit(entryPoint,TLSStart,TLSSize,argc,argv)
	call	__libc_preinit

	// it returns the entrypoint; 0 if we're the initial thread
	test	%eax,%eax
	je		initialThread

	// we're an additional thread, so call the desired function
	mov		%eax,%edx
	// get argument
	pop		%eax
	call	*%edx
	jmp		threadExit

	// initial thread calls main
initialThread:
	// init env stuff
	mov		8(%esp),%eax
	mov		12(%esp),%edx
	call	initenv

	// call function in .init-section
	call	_init

	// get argc and argv
	pop		%eax
	pop		%edx
	// finally, call main
	call	main

threadExit:
	// eax is already the exit-code
	call	exit
	// just to be sure
	1: jmp	1b

// all signal-handler return to this "function"
sigRetFunc:
	// we have to use int instead of sysenter here because sysexit does not restore ecx,edx and eflags.
	// so, if we have started the signal during an interrupt/exception and were thus not prepared for
	// loosing one of those, we would get into trouble.
	mov		$ASM_SYSC_ACKSIG,%eax
	int		$ASM_IRQ_ACKSIG
	// never reached
#endif
