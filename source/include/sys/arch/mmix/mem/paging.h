/**
 * $Id: paging.h 900 2011-06-02 20:18:17Z nasmussen $
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef MMIX_PAGING_H_
#define MMIX_PAGING_H_

#include <esc/common.h>
#include <sys/arch/mmix/mem/addrspace.h>

/**
 * Virtual memory layout:
 * 0x0000000000000000: +-----------------------------------+   -----
 *                     |                text               |     |
 *                     +-----------------------------------+     |
 *                     |               rodata              |     |
 *         ---         +-----------------------------------+     |
 *                     |                data               |     |
 *          |          |                                   |
 *          v          |                ...                |     u
 *         ---         +-----------------------------------+     s
 *          ^          |                                   |     e
 *          |          |        user-stack thread n        |     r
 *                     |                                   |     a
 *         ---         +-----------------------------------+     r
 *                     |                ...                |     e
 *         ---         +-----------------------------------+     a
 *          ^          |                                   |
 *          |          |        user-stack thread 0        |     |
 *                     |                                   |     |
 * 0x2000000000000000: +-----------------------------------+     |
 *                     |  dynlinker (always first, if ex)  |     |
 *                     |           shared memory           |     |
 *                     |         shared libraries          |     |
 *                     |          shared lib data          |     |
 *                     |       thread local storage        |     |
 *                     |        memory mapped stuff        |     |
 *                     |        (in arbitrary order)       |     |
 * 0x4000000000000000: +-----------------------------------+   -----    -----
 *                     |       VFS global file table       |     |        |
 * 0x4000000000400000: +-----------------------------------+     k
 *                     |             VFS nodes             |     e    dynamically extending regions
 * 0x4000000000800000: +-----------------------------------+     r
 *                     |             sll nodes             |     n        |
 * 0x4000000002800000: +-----------------------------------+     e      -----
 *                     |                ...                |     l
 *                     |                                   |     |
 * 0x6000000000000000: +-----------------------------------+   -----
 *                     |                ...                |
 * 0x8000000000000000: +-----------------------------------+   -----
 *                     |         kernel code+data          |     |
 *                     +-----------------------------------+
 *          |          |             mm-stack              |    directly mapped space
 *          v          +-----------------------------------+
 *                     |                ...                |     |
 * 0xFFFFFFFFFFFFFFFF: +-----------------------------------+   -----
 */

/* beginning of the kernel-code */
#define KERNEL_START			((uintptr_t)0x8000000000000000)
/* beginning of the kernel-data-structures */
#define KERNEL_AREA				((uintptr_t)0x4000000000000000)
#define DIR_MAPPED_SPACE		((uintptr_t)0x8000000000000000)

/* number of used segments */
#define SEGMENT_COUNT			3
/* page-tables for each segments in root-location */
#define PTS_PER_SEGMENT			2

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT			(PAGE_SIZE / 8)

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr)	((size_t)((uintptr_t)(addr) / PAGE_SIZE / PT_ENTRY_COUNT))

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr)	((size_t)(((uintptr_t)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT))

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount)	(((size_t)(pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

/* area for global-file-table */
#define GFT_AREA				KERNEL_AREA
#define GFT_AREA_SIZE			(PAGE_SIZE * PT_ENTRY_COUNT)
/* area for vfs-nodes */
#define VFSNODE_AREA			(GFT_AREA + GFT_AREA_SIZE)
#define VFSNODE_AREA_SIZE		(PAGE_SIZE * PT_ENTRY_COUNT)
/* area for sll-nodes */
#define SLLNODE_AREA			(VFSNODE_AREA + VFSNODE_AREA_SIZE)
#define SLLNODE_AREA_SIZE		(PAGE_SIZE * PT_ENTRY_COUNT * 8)

/* start-address of the text */
#define TEXT_BEGIN				0x2000
/* start-address of the text in dynamic linker */
#define INTERP_TEXT_BEGIN		0x2000000000000000

/* free area for shared memory, tls, shared libraries, ... */
#define FREE_AREA_BEGIN			0x2000000000000000
#define FREE_AREA_END			0x4000000000000000

typedef struct {
	sAddressSpace *addrSpace;
	uint64_t rV;
	ulong ptables;
} sContext;

typedef sContext *tPageDir;

#endif /* MMIX_PAGING_H_ */
