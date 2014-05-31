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
	SET		$1,0
	SET		$2,0
	SET		$3,0
	SET		$4,0
	SET		$5,0
	// call __libc_preinit(entryPoint,TLSStart,TLSSize,argc,argv)
	PUSHJ	$0,__libc_preinit

	// init c library
	PUSHJ	$0,__libc_init

	// init env stuff
	SET		$1,0
	SET		$2,0
	PUSHJ	$0,initenv

	// get argc and argv
	SET		$1,0
	SET		$2,0
	// finally, call main
	PUSHJ	$0,main

	// we should not reach this
1:	JMP		1b

// provide just a dummy
sigRetFunc:
	JMP		sigRetFunc

// we need the stack-pointer and frame-pointer here for forking
sp GREG
fp GREG
