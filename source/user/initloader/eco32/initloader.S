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

.global _start
.global sigRetFunc
.extern __libc_init
.extern main

_start:
	// setup a small stackframe
	sub		$29,$29,32
	add		$4,$0,$0
	add		$5,$0,$0
	add		$6,$0,$0
	add		$7,$0,$0
	// pass argv via stack
	stw		$8,$29,16
	// call __libc_preinit(entryPoint,TLSStart,TLSSize,argc,argv)
	jal		__libc_preinit

	// init c library
	jal		__libc_init

	// init env stuff
	add		$4,$0,$0
	add		$5,$0,$0
	jal		initenv

	// get argc and argv
	add		$4,$0,$0
	add		$5,$0,$0
	// finally, call main
	jal		main

	// we should not reach this
1:	j		1b

// provide just a dummy
sigRetFunc:
	j		sigRetFunc
