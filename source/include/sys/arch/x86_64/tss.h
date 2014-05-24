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

#pragma once

#include <sys/common.h>

/* the Task State Segment */
struct TSS {
	/* the size of the io-map (in bits) */
	static const size_t IO_MAP_SIZE					= 0xFFFF;

	static const uint16_t IO_MAP_OFFSET				= 104;
	/* an invalid offset for the io-bitmap => not loaded yet */
	static const uint16_t IO_MAP_OFFSET_INVALID		= 104 + 16;

	TSS() {
		ioMapOffset = IO_MAP_OFFSET_INVALID;
		ioMapEnd = 0xFF;
	}

	void setSP(uintptr_t sp) {
		rsp0 = sp;
	}

	uint32_t : 32; /* reserved */
	/* stack pointer for privilege levels 0-2 */
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint32_t : 32; /* reserved */
	uint32_t : 32; /* reserved */
	/* interrupt stack table pointer */
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint32_t : 32; /* reserved */
	uint32_t : 32; /* reserved */
	uint16_t : 16; /* reserved */
	/* Contains a 16-bit offset from the base of the TSS to the I/O permission bit map
	 * and interrupt redirection bitmap. When present, these maps are stored in the
	 * TSS at higher addresses. The I/O map base address points to the beginning of the
	 * I/O permission bit map and the end of the interrupt redirection bit map. */
	uint16_t ioMapOffset;
	uint8_t ioMap[IO_MAP_SIZE / 8];
	uint8_t ioMapEnd;
} A_PACKED;
