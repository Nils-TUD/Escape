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
	sub		$29,$29,4						// create stack frame
	stw		$31,$29,0						// save return register
	add		$12,$0,$0						// clear error-code
	trap
	beq		$12,$0,1f
	jal		errno_location
	sub		$9,$0,$12
	stw		$9,$2,0							// store to errno
	add		$2,$12,$0						// return error-code
1:
	ldw		$31,$29,0						// restore return register
	add		$29,$29,4						// release stack frame
	jr		$31

.global syscall4
.type syscall4, @function
syscall4:
	sub		$29,$29,4						// create stack frame
	stw		$31,$29,0						// save return register
	add		$12,$0,$0						// clear error-code
	ldw		$8,$29,20						// load arg 4 into $8
	trap
	beq		$12,$0,1f
	jal		errno_location
	sub		$9,$0,$12
	stw		$9,$2,0							// store to errno
	add		$2,$12,$0						// return error-code
1:
	ldw		$31,$29,0						// restore return register
	add		$29,$29,4						// release stack frame
	jr		$31

.global syscall7
.type syscall7, @function
syscall7:
	sub		$29,$29,4						// create stack frame
	stw		$31,$29,0						// save return register
	add		$12,$0,$0						// clear error-code
	ldw		$8,$29,20						// load arg 4 into $8
	ldw		$9,$29,24						// load arg 5 into $9
	ldw		$10,$29,28						// load arg 6 into $10
	ldw		$11,$29,32						// load arg 7 into $11
	trap
	beq		$12,$0,1f
	jal		errno_location
	sub		$9,$0,$12
	stw		$9,$2,0							// store to errno
	add		$2,$12,$0						// return error-code
1:
	ldw		$31,$29,0						// restore return register
	add		$29,$29,4						// release stack frame
	jr		$31
