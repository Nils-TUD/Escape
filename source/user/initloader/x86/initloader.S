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

#include <esc/arch.h>

.section .text

.global _start
.global sigRetFunc
.extern __libc_init
.extern main

_start:
	mov		$0,%ARG_1
	mov		$0,%ARG_2
	mov		$0,%ARG_3
#if defined(__x86_64__)
	mov		$0,%ARG_4
	mov		$0,%ARG_5
#else
	push	$0
	push	$0
#endif
	// call __libc_preinit(entryPoint,TLSStart,TLSSize,argc,argv)
	call	__libc_preinit
#if defined(__i586__)
	add		$WORDSIZE * 2,%REG(sp)
#endif

	// init c library
	call	__libc_init

	// init env stuff
	mov		$0,%ARG_1
	mov		$0,%ARG_2
	call	initenv

	// get argc and argv
	mov		$0,%ARG_1
	mov		$0,%ARG_2
	// finally, call main
	call	main

	// we should not reach this
1:	jmp		1b

// provide just a dummy
sigRetFunc:
	jmp		sigRetFunc
