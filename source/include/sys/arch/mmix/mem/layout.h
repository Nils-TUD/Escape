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

#include <sys/arch/mmix/mem/physmem.h>

/**
 * Virtual memory layout:
 * 0x0000000000000000: +-----------------------------------+   -----
 *                     |                text               |     |
 *                     +-----------------------------------+     |
 *                     |               rodata              |     |
 *         ---         +-----------------------------------+     |
 *                     |                data               |     |
 *          |          |                                   |     |
 *          v          |                                   |     |
 * 0x2000000000000000: +-----------------------------------+     |
 *                     |                                   |     |
 *          |          |      register-stack thread 0      |     |
 *          v          |                                   |     |
 *         ---         +-----------------------------------+     |
 *          ^          |                                   |     |
 *          |          |      software-stack thread 0      |     |
 *                     |                                   |     |
 *         ---         +-----------------------------------+     |
 *                     |                ...                |     |
 *         ---         +-----------------------------------+     |
 *                     |                                   |     |
 *          |          |      register-stack thread n      |     |
 *          v          |                                   |     |
 *         ---         +-----------------------------------+     |
 *          ^          |                                   |     |
 *          |          |      software-stack thread n      |     |
 *                     |                                   |     |
 * 0x4000000000000000: +-----------------------------------+     |
 *                     |  dynlinker (always first, if ex)  |     |
 *                     |           shared memory           |     |
 *                     |         shared libraries          |     |
 *                     |          shared lib data          |     |
 *                     |       thread local storage        |     |
 *                     |        memory mapped stuff        |     |
 *                     |        (in arbitrary order)       |     |
 * 0x6000000000000000: +-----------------------------------+   -----
 *                     |                ...                |
 * 0x8000000000000000: +-----------------------------------+   -----
 *                     |                                   |     |
 *                     |         kernel code+data          |
 *                     |                                   |    directly mapped space
 *                     +-----------------------------------+
 *                     |                ...                |     |
 * 0xFFFFFFFFFFFFFFFF: +-----------------------------------+   -----
 */

/* beginning of the kernel-code */
#define KERNEL_START			0x8000000000000000
#define KERNEL_AREA				KERNEL_START

/* all physical memory is directly mapped @ 0x8000000000000000 by the HW. so, put all of that in
 * the lower memory pool */
#define DIR_MAP_AREA			0x8000000000000000
#define DIR_MAP_AREA_SIZE		0x0000FFFF00000000

/* number of used segments */
#define SEGMENT_COUNT			3
/* page-tables for each segments in root-location */
#define PTS_PER_SEGMENT			2

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT			(PAGE_SIZE / 8)

/* on mmix, the dynamically extending regions are not mapped into the virtual memory. because
 * if we did so, we would have to copy all page-tables for that on every process-clone (the
 * address-space-number has to match rV.n). thus, we provide a different strategy for these
 * areas, that access the stuff over the directly mapped space. therefore, only the sizes are
 * important here. the start-addresses are ignored */
#define GFT_AREA				0
#define VFSNODE_AREA			0
#define SLLNODE_AREA			0
#define GFT_AREA_SIZE			(PAGE_SIZE * PT_ENTRY_COUNT)
#define VFSNODE_AREA_SIZE		(PAGE_SIZE * PT_ENTRY_COUNT)
#define SLLNODE_AREA_SIZE		(PAGE_SIZE * PT_ENTRY_COUNT * 8)

/* start-address of the text in dynamic linker */
#define INTERP_TEXT_BEGIN		0x4000000000000000

/* free area for shared memory, tls, shared libraries, ... */
#define FREE_AREA_BEGIN			0x4000000000000000
#define FREE_AREA_END			0x6000000000000000

/* the stack-area grows upwards in the second segment */
#define STACK_AREA_GROWS_DOWN	0
#define STACK_AREA_BEGIN		0x2000000000000000
#define STACK_AREA_END			0x4000000000000000
