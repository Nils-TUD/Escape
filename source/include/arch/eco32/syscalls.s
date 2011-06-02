#
# $Id: syscalls.s 833 2010-10-02 14:17:28Z nasmussen $
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

# macros for the different syscall-types (void / ret-value, arg-count, error-handling)

.macro SYSC_VOID_0ARGS name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	trap
	jr		$31
.endm

.macro SYSC_VOID_1ARGS name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	trap
	jr		$31
.endm

.macro SYSC_RET_0ARGS name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	trap
	jr		$31
.endm

.macro SYSC_RET_0ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	trap
	beq		$11,$0,1f
	add		$8,$0,errno
	stw		$11,$8,0							# store to errno
	add		$2,$11,$0							# return error-code
1:
	jr		$31
.endm

.macro SYSC_RET_1ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	trap
	beq		$11,$0,1f
	add		$8,$0,errno
	stw		$11,$8,0							# store to errno
	add		$2,$11,$0							# return error-code
1:
	jr		$31
.endm

.macro SYSC_RET_2ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	trap
	beq		$11,$0,1f
	add		$8,$0,errno
	stw		$11,$8,0							# store to errno
	add		$2,$11,$0							# return error-code
1:
	jr		$31
.endm

.macro SYSC_RET_3ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	trap
	beq		$11,$0,1f
	add		$8,$0,errno
	stw		$11,$8,0							# store to errno
	add		$2,$11,$0							# return error-code
1:
	jr		$31
.endm

.macro SYSC_RET_4ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	trap
	beq		$11,$0,1f
	add		$8,$0,errno
	stw		$11,$8,0							# store to errno
	add		$2,$11,$0							# return error-code
1:
	jr		$31
.endm

.macro SYSC_RET_5ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	ldw		$8,$29,16							# load arg 5 into $8
	trap
	beq		$11,$0,1f
	add		$8,$0,errno
	stw		$11,$8,0							# store to errno
	add		$2,$11,$0							# return error-code
1:
	jr		$31
.endm

.macro SYSC_RET_7ARGS_ERR name syscno
.global \name
.type \name, @function
\name:
	add		$2,$0,\syscno					# set syscall-number
	ldw		$8,$29,16							# load arg 5 into $8
	ldw		$9,$29,20							# load arg 6 into $9
	ldw		$10,$29,24						# load arg 7 into $10
	trap
	beq		$11,$0,1f
	add		$8,$0,errno
	stw		$11,$8,0							# store to errno
	add		$2,$11,$0							# return error-code
1:
	jr		$31
.endm

