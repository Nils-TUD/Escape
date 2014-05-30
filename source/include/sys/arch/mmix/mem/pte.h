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

/* number of used segments */
#define SEGMENT_COUNT			3
/* page-tables for each segments in root-location */
#define PTS_PER_SEGMENT			2

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT			(PAGE_SIZE / 8)

/* not used, but in order to compile the pagetables module */
#define PT_LEVELS					4
#define PT_BPL						10
#define PT_BITS						64

#define PTE_PRESENT					PTE_READABLE
#define PTE_LARGE					0
#define PTE_NOTSUPER				0
#define PTE_GLOBAL					0
#define PTE_NO_EXEC					0

/*
 * PTE:
 * +-------+-+-------------------------------------------+----------+-+-+-+
 * |       |e|                 frameNumber               |     n    |r|w|x|
 * +-------+-+-------------------------------------------+----------+-+-+-+
 * 63     57 56                                          13         3     0
 *
 * PTP:
 * +-+---------------------------------------------------+----------+-+-+-+
 * |1|                   ptframeNumber                   |     n    | ign |
 * +-+---------------------------------------------------+----------+-+-+-+
 * 63                                                    13         3     0
 */

#define PTE_EXISTS					(1UL << 56)
#define PTE_READABLE				(1UL << 2)
#define PTE_WRITABLE				(1UL << 1)
#define PTE_EXECUTABLE				(1UL << 0)
#define PTE_FRAMENO(pte)			(((pte) >> PAGE_BITS) & 0x7FFFFFFFFFF)
#define PTE_FRAMENO_MASK			0x00FFFFFFFFFFE000
#define PTE_NMASK					0x0000000000001FF8

#define PAGE_NO(virt)				(((uintptr_t)(virt) & 0x1FFFFFFFFFFFFFFF) >> PAGE_BITS)
#define SEGSIZE(rV,i)				((i) == 0 ? 0 : (((rV) >> (64 - (i) * 4)) & 0xF))

#if defined(__cplusplus)
typedef unsigned long pte_t;
#endif
