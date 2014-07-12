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

/* the Task State Segment */
struct TSS {
	/* the size of the io-map (in bits) */
	static const size_t IO_MAP_SIZE					= 0xFFFF;

	static const uint16_t IO_MAP_OFFSET				= 104;
	/* an invalid offset for the io-bitmap => not loaded yet */
	static const uint16_t IO_MAP_OFFSET_INVALID		= 104 + 16;

	TSS() {
		ss0 = 0x10;
		ioMapOffset = IO_MAP_OFFSET_INVALID;
		ioMapEnd = 0xFF;
	}

	void setSP(uintptr_t sp) {
		esp0 = sp;
	}

	/*
	 * Contains the segment selector for the TSS of the
	 * previous task (updated on a task switch that was initiated by a call, interrupt, or
	 * exception). This field (which is sometimes called the back link field) permits a
	 * task switch back to the previous task by using the IRET instruction.
	 */
	uint16_t prevTaskLink;
	uint16_t : 16; /* reserved */
	/* esp for PL 0 */
	uint32_t esp0;
	/* stack-segment for PL 0 */
	uint16_t ss0;
	uint16_t : 16; /* reserved */
	/* esp for PL 1 */
	uint32_t esp1;
	/* stack-segment for PL 1 */
	uint16_t ss1;
	uint16_t : 16; /* reserved */
	/* esp for PL 2 */
	uint32_t esp2;
	/* stack-segment for PL 2 */
	uint16_t ss2;
	uint16_t : 16; /* reserved */
	/* page-directory-base-register */
	uint32_t cr3;
	/* State of the EIP register prior to the task switch. */
	uint32_t eip;
	/* State of the EFAGS register prior to the task switch. */
	uint32_t eflags;
	/* general purpose registers */
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	/* segment registers */
	uint16_t es;
	uint16_t : 16; /* reserved */
	uint16_t cs;
	uint16_t : 16; /* reserved */
	uint16_t ss;
	uint16_t : 16; /* reserved */
	uint16_t ds;
	uint16_t : 16; /* reserved */
	uint16_t fs;
	uint16_t : 16; /* reserved */
	uint16_t gs;
	uint16_t : 16; /* reserved */
	/* Contains the segment selector for the task's LDT. */
	uint16_t ldtSegmentSelector;
	uint16_t : 16; /* reserved */
	/* When set, the T flag causes the processor to raise a debug exception when a task
	 * switch to this task occurs */
	uint16_t debugTrap	: 1,
					: 15; /* reserved */
	/* Contains a 16-bit offset from the base of the TSS to the I/O permission bit map
	 * and interrupt redirection bitmap. When present, these maps are stored in the
	 * TSS at higher addresses. The I/O map base address points to the beginning of the
	 * I/O permission bit map and the end of the interrupt redirection bit map. */
	uint16_t ioMapOffset;
	uint8_t ioMap[IO_MAP_SIZE / 8];
	uint8_t ioMapEnd;
} A_PACKED;
