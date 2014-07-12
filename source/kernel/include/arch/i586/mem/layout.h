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

#include <arch/i586/mem/physmem.h>
#include <arch/x86/mem/pte.h>

/**
 * Virtual memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                text               |     |
 *             +-----------------------------------+     |
 *             |               rodata              |     |
 *     ---     +-----------------------------------+     |
 *             |                data               |     |
 *      |      |                                   |     |
 *      v      |                ...                |     |
 *     ---     +-----------------------------------+
 *      ^      |                                   |     u
 *      |      |        user-stack thread n        |     s
 *             |                                   |     e
 *     ---     +-----------------------------------+     r
 *             |                ...                |     a
 *     ---     +-----------------------------------+     r
 *      ^      |                                   |     e
 *      |      |        user-stack thread 0        |     a
 *             |                                   |
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
 *             +-----------------------------------+     |
 *             |                ...                |     |
 * 0xC0800000: +-----------------------------------+     |
 *             |                                   |     |
 *      |      |            kernel-heap            |
 *      v      |                                   |     k
 * 0xC1800000: +-----------------------------------+     e      -----
 *             |       VFS global file table       |     r        |
 * 0xC1C00000: +-----------------------------------+     n
 *             |             VFS nodes             |     e    dynamically extending regions
 * 0xC2000000: +-----------------------------------+     l
 *             |             sll nodes             |     a        |
 * 0xC4000000: +-----------------------------------+     r      -----
 *             |                                   |     e
 *      |      |             free area             |     a
 *      v      |                                   |
 * 0xD0000000: +-----------------------------------+     |
 *             |                                   |     |
 *             |          physical memory          |     |
 *             |                                   |     |
 * 0xFE000000: +-----------------------------------+     |      -----
 *             |           temp map page           |     |        |
 * 0xFE001000: +-----------------------------------+     |    not shared page-tables
 *             |           kernel-stacks           |     |        |
 * 0xFFFFFFFF: +-----------------------------------+   -----    -----
 */

/* the virtual address of the kernel-area */
#define KERNEL_AREA				0xC0000000
/* the virtual address of the kernel */
#define KERNEL_BEGIN			0xC0000000
#define KERNEL_START			(KERNEL_BEGIN + KERNEL_P_ADDR)

/* maximum size of kernel code and data */
#define KERNEL_SIZE				(PT_SIZE * 2)

/* the kernel-heap */
#define KHEAP_START				(KERNEL_AREA + KERNEL_SIZE)
#define KHEAP_SIZE				(PT_SIZE * 4)

/* area for global-file-table */
#define GFT_AREA				(KHEAP_START + KHEAP_SIZE)
#define GFT_AREA_SIZE			PT_SIZE
/* area for vfs-nodes */
#define VFSNODE_AREA			(GFT_AREA + GFT_AREA_SIZE)
#define VFSNODE_AREA_SIZE		PT_SIZE
/* area for sll-nodes */
#define SLLNODE_AREA			(VFSNODE_AREA + VFSNODE_AREA_SIZE)
#define SLLNODE_AREA_SIZE		(PT_SIZE * 8)

/* this area is not managed, but we map all stuff one after another and never unmap it */
/* this is used for multiboot-modules, pmem, ACPI, ... */
#define KFREE_AREA				(SLLNODE_AREA + SLLNODE_AREA_SIZE)
/* use up all space till kernel-stacks and temp area */
#define KFREE_AREA_SIZE			(PT_SIZE * 64 - (			\
 									KERNEL_SIZE +			\
 									KHEAP_SIZE +			\
 									GFT_AREA_SIZE +			\
 									VFSNODE_AREA_SIZE +		\
 									SLLNODE_AREA_SIZE))

/* the area where we map the first part of the physical memory contiguously */
#define DIR_MAP_AREA			(KFREE_AREA + KFREE_AREA_SIZE)
#define DIR_MAP_AREA_SIZE		(TEMP_MAP_PAGE - DIR_MAP_AREA)

/* for temporary mappings */
#define TEMP_MAP_PAGE			(KSTACK_AREA - PAGE_SIZE)

/* our kernel-stacks */
#define KSTACK_AREA				(0xFFFFFFFF - (KSTACK_AREA_SIZE - 1))
#define KSTACK_AREA_SIZE		(PT_SIZE * 8 - PAGE_SIZE)

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
