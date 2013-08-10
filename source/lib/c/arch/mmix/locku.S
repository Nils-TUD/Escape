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

.section .text

.global locku
.global unlocku
.extern yield

# void locku(tULock *l)
locku:
	GET		$1,rJ
1:
	PUT		rP,0
	SET		$2,1
	CSWAP	$2,$0,0
	BNZ		$2,1f
	PUSHJ	$2,yield
	JMP		1b
1:
	PUT		rJ,$1
	POP		0,0

# void unlocku(tULock *l)
unlocku:
	STCO	0,$0,0
	POP		0,0
