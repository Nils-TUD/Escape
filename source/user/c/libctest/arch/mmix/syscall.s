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
	SET		$9,$0
	SET		$0,$1
	SET		$1,$2
	SET		$2,$3
	SET		$3,$4
	SET		$4,$5
	SET		$5,$6
	SET		$6,$7
	SET		$7,$8
	SETH	$8,#0000
	ORML	$8,#0000
	SLU		$9,$9,8
	OR		$8,$8,$9					# TRAP 0,$9,0
	PUT		rX,$8
	GETA	$8,@+12
	PUT		rW,$8
	RESUME	0
	BZ		$2,1f						# no-error?
	SET		$0,$2
1:
	POP		1,0

# int doSyscall(uint syscallNo,uint arg1,uint arg2,uint arg3)
doSyscall:
	SET		$9,$0
	SET		$0,$1
	SET		$1,$2
	SET		$2,$3
	SETH	$8,#0000
	ORML	$8,#0000
	SLU		$9,$9,8
	OR		$8,$8,$9					# TRAP 0,$9,0
	PUT		rX,$8
	GETA	$8,@+12
	PUT		rW,$8
	RESUME	0
	BZ		$2,1f						# no-error?
	SET		$0,$2
1:
	POP		1,0
