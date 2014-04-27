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
//  |     TLSStart     |  (0 if not present)
//  +------------------+
//  |     TLSSize      |  (0 if not present)
//  +------------------+
//  |    EntryPoint    |  (0 for initial thread, thread-entrypoint for others)
//  +------------------+

#ifndef SHAREDLIB
_start:
	ldw		$4,$29,0			// entrypoint
	ldw		$5,$29,4			// TLS start
	ldw		$6,$29,8			// TLS size
	ldw		$7,$29,12			// argc
	bne		$4,$0,1f			// skip that for threads
	ldw		$8,$29,16			// argv
1:
	// setup a small stackframe
	sub		$29,$29,32
	// pass argv via stack
	stw		$8,$29,16
	// call __libc_preinit(entryPoint,TLSStart,TLSSize,argc,argv)
	jal		__libc_preinit
	add		$29,$29,12
	// it returns the entrypoint; 0 if we're the initial thread
	beq		$2,$0,initialThread
	// we're an additional thread, so call the desired function
	ldw		$4,$29,32
	jalr	$2
	j		threadExit

	// initial thread calls main
initialThread:
	// init env stuff
	ldw		$4,$29,40		// envc
	ldw		$5,$29,44		// envv
	jal		initenv

	// call function in .init-section
	jal		_init

	// finally, call main
	ldw		$4,$29,32		// argc
	ldw		$5,$29,36		// argv
	jal		main

threadExit:
	add		$4,$2,$0
	jal		exit
	// just to be sure
	1: j	1b

// all signal-handler return to this "function"
sigRetFunc:
	add		$4,$0,ASM_SYSC_ACKSIG
	trap
	// never reached
#endif
