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

#include <common.h>

/* the descriptor table */
struct DescTable {
	uint16_t size;		/* the size of the table -1 (size=0 is not allowed) */
	ulong offset;
} A_PACKED;

/* a descriptor */
struct Desc {
	enum {
		SYS_TASK_GATE	= 0x05,
		SYS_TSS			= 0x09,
		SYS_INTR_GATE	= 0x0E,
		DATA_RO			= 0x10,
		DATA_RW			= 0x12,
		CODE_X			= 0x18,
		CODE_XR			= 0x1A,
	};

	enum {
		DPL_KERNEL		= 0x0,
		DPL_USER		= 0x3,
	};

	enum {
		BITS_32			= 0 << 5,
		BITS_64			= 1 << 5,
	};

	/**
	 * size:		If 0 the selector defines 16 bit protected mode. If 1 it defines 32 bit
	 * 				protected mode. You can have both 16 bit and 32 bit selectors at once.
	 */
	enum {
		SIZE_16			= 0 << 6,
		SIZE_32			= 1 << 6,
	};

	/**
	 * granularity:	If 0 the limit is in 1 B blocks (byte granularity), if 1 the limit is in
	 * 				4 KiB blocks (page granularity).
	 */
	enum {
		GRANU_BYTES		= 0 << 7,
		GRANU_PAGES		= 1 << 7,
	};

	/* limit[0..15] */
	uint16_t limitLow;

	/* address[0..15] */
	uint16_t addrLow;

	/* address[16..23] */
	uint8_t addrMiddle;

	/*
	 * present:		This must be 1 for all valid selectors.
	 * dpl:			Contains the ring level, 0 = highest (kernel), 3 = lowest (user applications).
	 * type:		segment type
	 */
	uint8_t type : 5,
			dpl : 2,
			present : 1;

	/* address[24..31] and other fields, depending on the type of descriptor */
	uint16_t addrHigh;
} A_PACKED;

struct Desc64 : public Desc {
	uint32_t addrUpper;
	uint32_t : 32;
} A_PACKED;
