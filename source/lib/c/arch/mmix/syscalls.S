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

.extern errno_location

.global syscall0
.type syscall0, @function
syscall0:
.global syscall1
.type syscall1, @function
syscall1:
.global syscall2
.type syscall2, @function
syscall2:
.global syscall3
.type syscall3, @function
syscall3:
.global syscall4
.type syscall4, @function
syscall4:
.global syscall7
.type syscall7, @function
syscall7:
	SET		$8,0						// clear error-code
	TRAP	0,0,0
	BZ		$8,1f						// no-error?
	GET		$9,rJ
	PUSHJ	$10,errno_location
	NEG		$1,0,$8
	STTU	$1,$10,0
	SET		$0,$8
	PUT		rJ,$9
1:
	POP		1,0							// return value is in $0
