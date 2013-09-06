#
# $Id$
# Copyright (C) 2008 - 2009 Nils Asmussen
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

.global syscalldbg
.type syscalldbg, @function
syscalldbg:
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
	add		$12,$0,$0						# clear error-code
	trap
	beq		$12,$0,1f
	add		$8,$0,errno
	sub		$9,$0,$12
	stw		$9,$8,0							# store to errno
	add		$2,$12,$0						# return error-code
1:
	jr		$31

.global syscall4
.type syscall4, @function
syscall4:
	add		$12,$0,$0						# clear error-code
	ldw		$8,$29,16						# load arg 4 into $8
	trap
	beq		$12,$0,1f
	add		$8,$0,errno
	sub		$9,$0,$12
	stw		$9,$8,0							# store to errno
	add		$2,$12,$0						# return error-code
1:
	jr		$31

.global syscall7
.type syscall7, @function
syscall7:
	add		$12,$0,$0						# clear error-code
	ldw		$8,$29,16						# load arg 4 into $8
	ldw		$9,$29,20						# load arg 5 into $9
	ldw		$10,$29,24						# load arg 6 into $10
	ldw		$11,$29,28						# load arg 7 into $11
	trap
	beq		$12,$0,1f
	add		$8,$0,errno
	sub		$9,$0,$12
	stw		$9,$8,0							# store to errno
	add		$2,$12,$0						# return error-code
1:
	jr		$31
