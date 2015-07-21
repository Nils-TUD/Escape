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

#include <sys/asm.h>

// uint16_t cputole16(uint16_t in);
// uint16_t le16tocpu(uint16_t in);
BEGIN_FUNC(cputole16)
BEGIN_FUNC(le16tocpu)
	.nosyn
	and		$2,$4,0xFF00
	slr		$2,$2,8
	and		$8,$4,0xFF
	sll		$8,$8,8
	or		$2,$2,$8
	jr		$31
	.syn
END_FUNC(le16tocpu)
END_FUNC(cputole16)

// uint32_t cputole32(uint32_t in);
// uint32_t le32tocpu(uint32_t in);
BEGIN_FUNC(cputole32)
BEGIN_FUNC(le32tocpu)
	.nosyn
	and		$2,$4,0xFF00
	sll		$2,$2,8
	and		$8,$4,0xFF
	sll		$8,$8,24
	or		$2,$2,$8
	slr		$8,$4,8
	and		$8,$8,0xFF00
	or		$2,$2,$8
	slr		$8,$4,24
	or		$2,$2,$8
	jr		$31
	.syn
END_FUNC(le32tocpu)
END_FUNC(cputole32)
