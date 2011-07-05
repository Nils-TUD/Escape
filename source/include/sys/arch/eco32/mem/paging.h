/**
 * $Id$
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

#ifndef ECO32_PAGING_H_
#define ECO32_PAGING_H_

#include <esc/common.h>

/**
 * Virtual memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                text               |     |
 *             +-----------------------------------+     |
 *             |               rodata              |     |
 *     ---     +-----------------------------------+     |
 *             |                data               |     |
 *      |      |                                   |
 *      v      |                ...                |     u
 *     ---     +-----------------------------------+     s
 *      ^      |                                   |     e
 *      |      |        user-stack thread n        |     r
 *             |                                   |     a
 *     ---     +-----------------------------------+     r
 *             |                ...                |     e
 *     ---     +-----------------------------------+     a
 *      ^      |                                   |
 *      |      |        user-stack thread 0        |     |
 *             |                                   |     |
 * 0x60000000: +-----------------------------------+     |
 *             |  dynlinker (always first, if ex)  |     |
 *             |           shared memory           |     |
 *             |         shared libraries          |     |
 *             |          shared lib data          |     |
 *             |       thread local storage        |     |
 *             |        memory mapped stuff        |     |
 *             |        (in arbitrary order)       |     |
 * 0x80000000: +-----------------------------------+   -----    -----
 *             |         mapped page-tables        |     |        |
 * 0x80400000: +-----------------------------------+     |     not shared pts
 *             |      temp mapped page-tables      |     |        |
 * 0x80800000: +-----------------------------------+            -----
 *             |                                   |     k
 *      |      |            kernel-heap            |     e
 *      v      |                                   |     r
 * 0x80C00000: +-----------------------------------+     r      -----
 *             |       VFS global file table       |     e        |
 * 0x81000000: +-----------------------------------+     a
 *             |             VFS nodes             |          dynamically extending regions
 * 0x81400000: +-----------------------------------+     |
 *             |             sll nodes             |     |        |
 * 0x83400000: +-----------------------------------+     |      -----
 *             |           temp map area           |     |
 * 0x84400000: +-----------------------------------+     |      -----
 *             |           kernel stack            |     |     not shared
 * 0x84401000: +-----------------------------------+     |      -----
 *             |                ...                |     |
 * 0xC0000000: +-----------------------------------+   -----
 *             |         kernel code+data          |     |
 *             +-----------------------------------+
 *      |      |             mm-stack              |    directly mapped space
 *      v      +-----------------------------------+
 *             |                ...                |     |
 * 0xFFFFFFFF: +-----------------------------------+   -----
 */

/* beginning of the kernel-code */
#define KERNEL_START			((uintptr_t)0xC0000000)
/* beginning of the kernel-data-structures */
#define KERNEL_AREA				((uintptr_t)0x80000000)
#define DIR_MAPPED_SPACE		((uintptr_t)0xC0000000)

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT			(PAGE_SIZE / 4)

/* the start of the mapped page-tables area */
#define MAPPED_PTS_START		KERNEL_AREA
/* the start of the temporary mapped page-tables area */
#define TMPMAP_PTS_START		(MAPPED_PTS_START + (PT_ENTRY_COUNT * PAGE_SIZE))

/* page-directories in virtual memory */
#define PAGE_DIR_AREA			(MAPPED_PTS_START + PAGE_SIZE * 512)
/* needed for building a new page-dir */
#define PAGE_DIR_TMP_AREA		(TMPMAP_PTS_START + PAGE_SIZE * 513)

/* the start of the kernel-heap */
#define KERNEL_HEAP_START		(TMPMAP_PTS_START + (PT_ENTRY_COUNT * PAGE_SIZE))
/* the size of the kernel-heap (4 MiB) */
#define KERNEL_HEAP_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE)

/* area for global-file-table */
#define GFT_AREA				(KERNEL_HEAP_START + KERNEL_HEAP_SIZE)
#define GFT_AREA_SIZE			(4 * M)
/* area for vfs-nodes */
#define VFSNODE_AREA			(GFT_AREA + GFT_AREA_SIZE)
#define VFSNODE_AREA_SIZE		(4 * M)
/* area for sll-nodes */
#define SLLNODE_AREA			(VFSNODE_AREA + VFSNODE_AREA_SIZE)
#define SLLNODE_AREA_SIZE		(32 * M)

/* for mapping some pages of foreign processes */
#define TEMP_MAP_AREA			(SLLNODE_AREA + SLLNODE_AREA_SIZE)
#define TEMP_MAP_AREA_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE * 4)

/* our kernel-stack */
#define KERNEL_STACK			(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE)

/* the size of the temporary stack we use at the beginning */
#define TMP_STACK_SIZE			PAGE_SIZE

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr)	((size_t)((uintptr_t)(addr) / PAGE_SIZE / PT_ENTRY_COUNT))

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr)	((size_t)(((uintptr_t)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT))

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount)	(((size_t)(pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

/* start-address of the text */
#define TEXT_BEGIN				0x1000
/* start-address of the text in dynamic linker */
#define INTERP_TEXT_BEGIN		0x60000000

/* free area for shared memory, tls, shared libraries, ... */
#define FREE_AREA_BEGIN			0x60000000
#define FREE_AREA_END			KERNEL_AREA

/* the stack-area grows downwards from the free area to data, text and so on */
#define STACK_AREA_GROWS_DOWN	1
#define STACK_AREA_END			FREE_AREA_BEGIN

/* determines whether the given address is on the heap */
#define IS_ON_HEAP(addr) ((uintptr_t)(addr) >= KERNEL_HEAP_START && \
		(uintptr_t)(addr) < KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

/* determines whether the given address is in a shared kernel area; in this case it is "shared"
 * if it is accessed over the directly mapped space. */
#define IS_SHARED(addr)			((uintptr_t)(addr) >= KERNEL_START || \
		((uintptr_t)(addr) >= KERNEL_HEAP_START && (uintptr_t)(addr) < KERNEL_STACK))

typedef uintptr_t tPageDir;

extern tPageDir curPDir;

/**
 * Sets the entry with index <index> to the given translation
 */
extern void tlb_set(int index,uint virt,uint phys);

/**
 * Prints the contents of the TLB
 */
void paging_printTLB(void);

#endif /* ECO32_PAGING_H_ */
