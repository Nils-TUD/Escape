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

#include <sys/arch/i586/mem/physmem.h>

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
 *             |                ...                |     k
 * 0xC0800000: +-----------------------------------+     e
 *             |                                   |     r
 *      |      |            kernel-heap            |     n
 *      v      |                                   |     e
 * 0xC0C00000: +-----------------------------------+     l
 *             |           temp map area           |     a
 * 0xC1C00000: +-----------------------------------+     r
 *             |                ...                |     e
 * 0xD0000000: +-----------------------------------+     a      -----
 *             |       VFS global file table       |              |
 * 0xD0400000: +-----------------------------------+     |
 *             |             VFS nodes             |     |    dynamically extending regions
 * 0xD0800000: +-----------------------------------+     |
 *             |             sll nodes             |     |        |
 * 0xD2800000: +-----------------------------------+     |      -----
 *             |                ...                |     |
 * 0xE0000000: +-----------------------------------+     |      -----
 *             |                                   |     |        |
 *      |      |             free area             |     |    unmanaged
 *      v      |                                   |     |        |
 * 0xEFFFFFFF: +-----------------------------------+     |      -----
 *             |                ...                |     |
 * 0xFD800000: +-----------------------------------+     |      -----
 *             |           kernel-stacks           |     |        |
 * 0xFF800000: +-----------------------------------+     |
 *             |     temp mapped page-tables       |     |    not shared page-tables (10)
 * 0xFFC00000: +-----------------------------------+     |
 *             |        mapped page-tables         |     |        |
 * 0xFFFFFFFF: +-----------------------------------+   -----    -----
 */

/* the virtual address of the kernel */
#define KERNEL_START			(0xC0000000 + KERNEL_P_ADDR)
/* the virtual address of the kernel-area */
#define KERNEL_AREA				0xC0000000

#define DIR_MAP_AREA			0xF0000000
#define DIR_MAP_AREA_SIZE		(64 * 1024 * 1024)

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT			(PAGE_SIZE / 4)

/* the start of the mapped page-tables area */
#define MAPPED_PTS_START		(0xFFFFFFFF - (PT_ENTRY_COUNT * PAGE_SIZE) + 1)
/* the start of the temporary mapped page-tables area */
#define TMPMAP_PTS_START		(MAPPED_PTS_START - (PT_ENTRY_COUNT * PAGE_SIZE))
/* the start of the kernel-heap */
#define KERNEL_HEAP_START		(KERNEL_AREA + (PT_ENTRY_COUNT * PAGE_SIZE) * 2)
/* the size of the kernel-heap (4 MiB) */
#define KERNEL_HEAP_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE /* * 4 */)

/* page-directories in virtual memory */
#define PAGE_DIR_AREA			(MAPPED_PTS_START + PAGE_SIZE * (PT_ENTRY_COUNT - 1))
/* needed for building a new page-dir */
#define PAGE_DIR_TMP_AREA		(TMPMAP_PTS_START + PAGE_SIZE * (PT_ENTRY_COUNT - 1))
/* our kernel-stack */
#define KERNEL_STACK_AREA		(TMPMAP_PTS_START - PAGE_SIZE * PT_ENTRY_COUNT * 8)
#define KERNEL_STACK_AREA_SIZE	(PAGE_SIZE * PT_ENTRY_COUNT * 8)

/* for mapping some pages of foreign processes */
#define TEMP_MAP_AREA			(KERNEL_HEAP_START + KERNEL_HEAP_SIZE)
#define TEMP_MAP_AREA_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE * 4)

/* this area is not managed, but we map all stuff one after another and never unmap it */
/* this is used for multiboot-modules, pmem, ACPI, ... */
#define FREE_KERNEL_AREA		0xE0000000
#define FREE_KERNEL_AREA_SIZE	(64 * PAGE_SIZE * PT_ENTRY_COUNT)

/* area for global-file-table */
#define GFT_AREA				0xD0000000
#define GFT_AREA_SIZE			(4 * 1024 * 1024)
/* area for vfs-nodes */
#define VFSNODE_AREA			(GFT_AREA + GFT_AREA_SIZE)
#define VFSNODE_AREA_SIZE		(4 * 1024 * 1024)
/* area for sll-nodes */
#define SLLNODE_AREA			(VFSNODE_AREA + VFSNODE_AREA_SIZE)
#define SLLNODE_AREA_SIZE		(32 * 1024 * 1024)

/* the size of the temporary stack we use at the beginning */
#define TMP_STACK_SIZE			PAGE_SIZE

/* start-address of the text in dynamic linker */
#define INTERP_TEXT_BEGIN		0xA0000000

/* free area for shared memory, tls, shared libraries, ... */
#define FREE_AREA_BEGIN			0xA0000000
#define FREE_AREA_END			KERNEL_AREA

/* the stack-area grows downwards from the free area to data, text and so on */
#define STACK_AREA_GROWS_DOWN	1
#define STACK_AREA_END			FREE_AREA_BEGIN
