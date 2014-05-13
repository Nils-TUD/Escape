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

#include <sys/arch/eco32/mem/physmem.h>

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
 *             |                                   |     |
 *             |         kernel code+data          |
 *             |                                   |    directly mapped space
 *             +-----------------------------------+
 *             |                ...                |     |
 * 0xFFFFFFFF: +-----------------------------------+   -----
 */

/* beginning of the kernel-code */
#define KERNEL_START			0xC0000000
/* beginning of the kernel-area */
#define KERNEL_AREA				0x80000000
#define DIR_MAPPED_SPACE		0xC0000000

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
#define GFT_AREA_SIZE			(4 * 1024 * 1024)
/* area for vfs-nodes */
#define VFSNODE_AREA			(GFT_AREA + GFT_AREA_SIZE)
#define VFSNODE_AREA_SIZE		(4 * 1024 * 1024)
/* area for sll-nodes */
#define SLLNODE_AREA			(VFSNODE_AREA + VFSNODE_AREA_SIZE)
#define SLLNODE_AREA_SIZE		(32 * 1024 * 1024)

/* for mapping some pages of foreign processes */
#define TEMP_MAP_AREA			(SLLNODE_AREA + SLLNODE_AREA_SIZE)
#define TEMP_MAP_AREA_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE * 4)

/* our kernel-stack */
#define KERNEL_STACK			(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE)

/* the size of the temporary stack we use at the beginning */
#define TMP_STACK_SIZE			PAGE_SIZE

/* start-address of the text in dynamic linker */
#define INTERP_TEXT_BEGIN		0x60000000

/* free area for shared memory, tls, shared libraries, ... */
#define FREE_AREA_BEGIN			0x60000000
#define FREE_AREA_END			KERNEL_AREA

/* the stack-area grows downwards from the free area to data, text and so on */
#define STACK_AREA_GROWS_DOWN	1
#define STACK_AREA_END			FREE_AREA_BEGIN
