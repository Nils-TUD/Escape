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

#include <arch/x86/mem/pte.h>
#include <arch/x86_64/mem/physmem.h>

/**
 * Virtual memory layout:
 * 0x0000000000000000: +-----------------------------------+   -----
 *                     |                text               |     |
 *                     +-----------------------------------+     |
 *                     |               rodata              |     |
 *         ---         +-----------------------------------+     |
 *                     |                data               |     |
 *          |          |                                   |     |
 *          v          |                ...                |     |
 *         ---         +-----------------------------------+
 *          ^          |                                   |     u
 *          |          |        user-stack thread n        |     s
 *                     |                                   |     e
 *         ---         +-----------------------------------+     r
 *                     |                ...                |     a
 *         ---         +-----------------------------------+     r
 *          ^          |                                   |     e
 *          |          |        user-stack thread 0        |     a
 *                     |                                   |
 * 0x0000000070000000: +-----------------------------------+     |
 *                     |  dynlinker (always first, if ex)  |     |
 *                     |           shared memory           |     |
 *                     |         shared libraries          |     |
 *                     |          shared lib data          |     |
 *                     |       thread local storage        |     |
 *                     |        memory mapped stuff        |     |
 *                     |        (in arbitrary order)       |     |
 * 0x00007E8000000000: +-----------------------------------+   -----
 *                     |                                   |     |
 *          |          |            kernel-heap            |
 *          v          |                                   |     k
 * 0x00007E8001000000: +-----------------------------------+     e      -----
 *                     |       VFS global file table       |     r        |
 * 0x00007E8001400000: +-----------------------------------+     n
 *                     |             VFS nodes             |     e    dynamically extending regions
 * 0x00007E8001800000: +-----------------------------------+     l
 *                     |             sll nodes             |     a        |
 * 0x00007E8003800000: +-----------------------------------+     r      -----
 *                     |                                   |     e
 *          |          |             free area             |     a
 *          v          |                                   |
 * 0x00007E800B800000: +-----------------------------------+     |
 *                     |                ...                |     |
 * 0x00007F0000000000: +-----------------------------------+     |      -----
 *                     |           temp map page           |     |        |
 * 0x00007F0000001000: +-----------------------------------+     |    not shared page-tables
 *                     |           kernel-stacks           |     |        |
 * 0x00007F0002001000: +-----------------------------------+     |      -----
 *                     |                ...                |     |
 * 0x00007F8000000000: +-----------------------------------+     |
 *                     |                                   |     |
 *                     |          physical memory          |     |
 *                     |                                   |     |
 * 0x0000800000000000: +-----------------------------------+     |
 *                     |                ...                |     |
 * 0xFFFFFFFF80100000: +-----------------------------------+     |
 *                     |         kernel code+data          |     |
 * 0xFFFFFFFF80600000: +-----------------------------------+     |
 *                     |                ...                |     |
 * 0xFFFFFFFFFFFFFFFF: +-----------------------------------+   -----
 */

/* the virtual address of the kernel-area */
#define KERNEL_AREA				0x00007E8000000000
/* the virtual address of the kernel */
#define KERNEL_BEGIN			0xFFFFFFFF80000000
#define KERNEL_START			(KERNEL_BEGIN + KERNEL_P_ADDR)

/* the kernel-heap */
#define KHEAP_START				KERNEL_AREA
#define KHEAP_SIZE				(PT_SIZE * 8)

/* area for global-file-table */
#define GFT_AREA				(KHEAP_START + KHEAP_SIZE)
#define GFT_AREA_SIZE			(PT_SIZE * 2)
/* area for vfs-nodes */
#define VFSNODE_AREA			(GFT_AREA + GFT_AREA_SIZE)
#define VFSNODE_AREA_SIZE		(PT_SIZE * 2)
/* area for sll-nodes */
#define SLLNODE_AREA			(VFSNODE_AREA + VFSNODE_AREA_SIZE)
#define SLLNODE_AREA_SIZE		(PT_SIZE * 16)

/* this area is not managed, but we map all stuff one after another and never unmap it */
/* this is used for multiboot-modules, pmem, ACPI, ... */
#define KFREE_AREA				(SLLNODE_AREA + SLLNODE_AREA_SIZE)
#define KFREE_AREA_SIZE			(PT_SIZE * 64)

/* for temporary mappings */
#define TEMP_MAP_PAGE			0x00007F0000000000

/* our kernel-stacks */
#define KSTACK_AREA				(TEMP_MAP_PAGE + PAGE_SIZE)
/* the max. number of threads is 8192 -> 16 page tables (TODO use that constant) */
#define KSTACK_AREA_SIZE		(16 * PT_SIZE)

/* the area where we map the first part of the physical memory contiguously */
#define DIR_MAP_AREA			0x00007F8000000000
/* 32 GiB should be enough for now (note: it can't be more than one PDPT) */
#define DIR_MAP_AREA_SIZE		(PD_SIZE * 32)

/* the size of the temporary stack we use at the beginning */
#define TMP_STACK_SIZE			PAGE_SIZE

/* start-address of the text in dynamic linker (put it below 2G to be able to use -mcmodel=small) */
#define INTERP_TEXT_BEGIN		0x0000000070000000

/* free area for shared memory, tls, shared libraries, ... */
#define FREE_AREA_BEGIN			0x0000000070000000
#define FREE_AREA_END			KERNEL_AREA

/* the stack-area grows downwards from the free area to data, text and so on */
#define STACK_AREA_GROWS_DOWN	1
#define STACK_AREA_END			FREE_AREA_BEGIN
