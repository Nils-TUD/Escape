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

#include <arch/eco32/mem/physmem.h>
#include <arch/eco32/mem/pte.h>

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
 *             |     not used in C++ code but      |     |   not shared pt
 *             |     only in TLB miss handlers     |     |        |
 * 0x80400000: +-----------------------------------+     |      -----
 *             |                                   |     |
 *      |      |            kernel-heap            |     |
 *      v      |                                   |
 * 0x80800000: +-----------------------------------+     k      -----
 *             |       VFS global file table       |     e        |
 * 0x80C00000: +-----------------------------------+     r
 *             |             VFS nodes             |     n    dynamically extending regions
 * 0x81000000: +-----------------------------------+     e
 *             |             sll nodes             |     l        |
 * 0x83000000: +-----------------------------------+     a      -----
 *             |           temp map area           |     r
 * 0x84000000: +-----------------------------------+     e      -----
 *             |           kernel stack            |     a     not shared
 * 0x84001000: +-----------------------------------+            -----
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
#define KERNEL_BEGIN			KERNEL_START
/* beginning of the kernel-area */
#define KERNEL_AREA				0x80000000

/* all physical memory is directly mapped @ 0xC0000000 by the HW. so, put all of that in the lower
 * memory pool */
#define DIR_MAP_AREA			0xC0000000
#define DIR_MAP_AREA_SIZE		(1024 * 1024 * 1024)

#define PT_SIZE					(PAGE_SIZE * PT_ENTRY_COUNT)

/* the start of the mapped page-tables area */
#define MAPPED_PTS_START		KERNEL_AREA

/* the start of the kernel-heap */
#define KHEAP_START				(KERNEL_AREA + PT_ENTRY_COUNT * PAGE_SIZE)
/* the size of the kernel-heap (4 MiB) */
#define KHEAP_SIZE				(PT_ENTRY_COUNT * PAGE_SIZE)

/* area for global-file-table */
#define GFT_AREA				(KHEAP_START + KHEAP_SIZE)
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
#define KSTACK_AREA				(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE)
#define KERNEL_STACK			KSTACK_AREA

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
