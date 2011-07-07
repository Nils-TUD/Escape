/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

.section	.text

.global		doSyscall7
.global		doSyscall

# int doSyscall7(uint syscallNo,uint arg1,uint arg2,uint arg3,uint arg4,uint arg5,uint arg6,uint arg7)
doSyscall7:
	add		$2,$0,$4
	add		$4,$0,$5
	add		$5,$0,$6
	add		$6,$0,$7
	ldw		$7,$29,16
	ldw		$8,$29,20
	ldw		$9,$29,24
	ldw		$10,$29,28
	trap
	beq		$11,$0,1f
	add		$2,$11,$0
1:
	jr		$31

# int doSyscall(uint syscallNo,uint arg1,uint arg2,uint arg3)
doSyscall:
	add		$2,$0,$4
	add		$4,$0,$5
	add		$5,$0,$6
	add		$6,$0,$7
	trap
	beq		$11,$0,1f
	add		$2,$11,$0
1:
	jr		$31
