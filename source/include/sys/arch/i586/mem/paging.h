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

#ifndef I586_PAGING_H_
#define I586_PAGING_H_

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
 * 0xA0000000: +-----------------------------------+     |
 *             |  dynlinker (always first, if ex)  |     |
 *             |           shared memory           |     |
 *             |         shared libraries          |     |
 *             |          shared lib data          |     |
 *             |       thread local storage        |     |
 *             |        memory mapped stuff        |     |
 *             |        (in arbitrary order)       |     |
 * 0xC0000000: +-----------------------------------+   -----
 *             |                ...                |     |
 * 0xC0100000: +-----------------------------------+     |
 *             |         kernel code+data          |     |
 *             +-----------------------------------+
 *      |      |             mm-stack              |     k
 *      v      +-----------------------------------+     e
 *             |                ...                |     r
 * 0xC0800000: +-----------------------------------+     n
 *             |                                   |     e
 *      |      |            kernel-heap            |     l
 *      v      |                                   |     a
 * 0xC1C00000: +-----------------------------------+     r
 *             |           temp map area           |     e
 * 0xC2800000: +-----------------------------------+     a
 *             |             APIC area             |     e
 * 0xC2801000: +-----------------------------------+     a
 *             |             TSS area              |     e
 * 0x????????: +-----------------------------------+     a
 *             |                ...                |
 * 0xD0000000: +-----------------------------------+     |      -----
 *             |       VFS global file table       |     |        |
 * 0xD0400000: +-----------------------------------+     |
 *             |             VFS nodes             |     |    dynamically extending regions
 * 0xD0800000: +-----------------------------------+     |
 *             |             sll nodes             |     |        |
 * 0xD2800000: +-----------------------------------+     |      -----
 *             |                ...                |     |
 * 0xFF7FF000: +-----------------------------------+     |      -----
 *             |            kernel-stack           |     |        |
 * 0xFF800000: +-----------------------------------+     |
 *             |     temp mapped page-tables       |     |    not shared page-tables (3)
 * 0xFFC00000: +-----------------------------------+     |
 *             |        mapped page-tables         |     |        |
 * 0xFFFFFFFF: +-----------------------------------+   -----    -----
 */

/* the virtual address of the kernel-area */
#define KERNEL_START			((uintptr_t)0xC0000000)
/* the virtual address of the kernel itself */
#define KERNEL_V_ADDR			(KERNEL_START + KERNEL_P_ADDR)

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT			(PAGE_SIZE / 4)

/* the start of the mapped page-tables area */
#define MAPPED_PTS_START		(0xFFFFFFFF - (PT_ENTRY_COUNT * PAGE_SIZE) + 1)
/* the start of the temporary mapped page-tables area */
#define TMPMAP_PTS_START		(MAPPED_PTS_START - (PT_ENTRY_COUNT * PAGE_SIZE))
/* the start of the kernel-heap */
#define KERNEL_HEAP_START		(KERNEL_START + (PT_ENTRY_COUNT * PAGE_SIZE) * 2)
/* the size of the kernel-heap (4 MiB) */
#define KERNEL_HEAP_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE /* * 4 */)

/* page-directories in virtual memory */
#define PAGE_DIR_AREA			(MAPPED_PTS_START + PAGE_SIZE * (PT_ENTRY_COUNT - 1))
/* needed for building a new page-dir */
#define PAGE_DIR_TMP_AREA		(TMPMAP_PTS_START + PAGE_SIZE * (PT_ENTRY_COUNT - 1))
/* our kernel-stack */
#define KERNEL_STACK			(TMPMAP_PTS_START - PAGE_SIZE)

/* for mapping some pages of foreign processes */
#define TEMP_MAP_AREA			(KERNEL_HEAP_START + KERNEL_HEAP_SIZE)
#define TEMP_MAP_AREA_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE * 4)

/* for mapping the APIC */
#define APIC_AREA				(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE)
#define APIC_AREA_SIZE			PAGE_SIZE

/* for mapping the TSS's for APs (have to be page-aligned) */
#define TSS_AREA				(APIC_AREA + APIC_AREA_SIZE)

/* area for global-file-table */
#define GFT_AREA				0xD0000000
#define GFT_AREA_SIZE			(4 * M)
/* area for vfs-nodes */
#define VFSNODE_AREA			(GFT_AREA + GFT_AREA_SIZE)
#define VFSNODE_AREA_SIZE		(4 * M)
/* area for sll-nodes */
#define SLLNODE_AREA			(VFSNODE_AREA + VFSNODE_AREA_SIZE)
#define SLLNODE_AREA_SIZE		(32 * M)

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
#define INTERP_TEXT_BEGIN		0xA0000000

/* free area for shared memory, tls, shared libraries, ... */
#define FREE_AREA_BEGIN			0xA0000000
#define FREE_AREA_END			KERNEL_START

/* the stack-area grows downwards from the free area to data, text and so on */
#define STACK_AREA_GROWS_DOWN	1
#define STACK_AREA_END			FREE_AREA_BEGIN

/* determines whether the given address is on the heap */
#define IS_ON_HEAP(addr)		((uintptr_t)(addr) >= KERNEL_HEAP_START && \
		(uintptr_t)(addr) < KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

/* determines whether the given address is in a shared kernel area */
#define IS_SHARED(addr)			((uintptr_t)(addr) >= KERNEL_START && (uintptr_t)(addr) < KERNEL_STACK)

typedef uintptr_t tPageDir;

/**
 * Activates paging
 */
void paging_activate(void);

/**
 * Reserves page-tables for the whole higher-half and inserts them into the page-directory.
 * This should be done ONCE at the beginning as soon as the physical memory management is set up
 */
void paging_mapKernelSpace(void);

/**
 * Unmaps the page-table 0. This should be used only by the GDT to unmap the first page-table as
 * soon as the GDT is setup for a flat memory layout!
 */
void paging_gdtFinished(void);

/**
 * Assembler routine to exchange the page-directory to the given one
 *
 * @param physAddr the physical address of the page-directory
 */
extern void paging_exchangePDir(tPageDir physAddr);

#endif /* I586_PAGING_H_ */
