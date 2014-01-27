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

.global le16tocpu
.global le32tocpu
.global cputole16
.global cputole32

# uint16_t cputole16(uint16_t in);
# uint16_t le16tocpu(uint16_t in);
cputole16:
le16tocpu:
	SETH	$1,#0102
	ORMH	$1,#0408
	ORML	$1,#1020
	ORL		$1,#4080
	MOR		$0,$0,$1
	SRU		$0,$0,48
	POP		1,0

# uint32_t cputole32(uint32_t in);
# uint32_t le32tocpu(uint32_t in);
cputole32:
le32tocpu:
	SETH	$1,#0102
	ORMH	$1,#0408
	ORML	$1,#1020
	ORL		$1,#4080
	MOR		$0,$0,$1
	SRU		$0,$0,32
	POP		1,0

