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

#ifndef MM_H_
#define MM_H_

#include <sys/common.h>
#include <sys/multiboot.h>

/**
 * Physical memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                                   |     |
 *             |                VM86               |     |
 *             |                                   |     |
 * 0x00100000: +-----------------------------------+     |
 *             |                                   |
 *             |          kernel code+data         |  unmanaged
 *             |                                   |
 *             +-----------------------------------+     |
 *             |         multiboot-modules         |     |
 *             +-----------------------------------+     |
 *             |              MM-stack             |     |
 *             +-----------------------------------+   -----
 *             |             MM-bitmap             |     |
 *             +-----------------------------------+     |
 *             |                                   |
 *             |                                   |  bitmap managed (2 MiB)
 *             |                ...                |
 *             |                                   |     |
 *             |                                   |     |
 *             +-----------------------------------+   -----
 *             |                                   |     |
 *             |                                   |
 *             |                ...                |  stack managed (remaining)
 *             |                                   |
 *             |                                   |     |
 * 0xFFFFFFFF: +-----------------------------------+   -----
 */

/* the physical start-address of the kernel-area */
#define KERNEL_AREA_P_ADDR		0x0
/* the physical start-address of the kernel itself */
#define KERNEL_P_ADDR			(1 * M)

#define PAGE_SIZE				(4 * K)
#define PAGE_SIZE_SHIFT			12

/* converts bytes to pages */
#define BYTES_2_PAGES(b)		(((u32)(b) + (PAGE_SIZE - 1)) >> PAGE_SIZE_SHIFT)

/* set values to support bit-masks of the types */
typedef enum {MM_CONT = 1,MM_DEF = 2} eMemType;

/**
 * Initializes the memory-management
 *
 * @param mb the multiboot-info
 */
void mm_init(const sMultiBoot *mb);

/**
 * @return the number of bytes used for the mm-stack
 */
u32 mm_getStackSize(void);

/**
 * Counts the number of free frames. This is primarly intended for debugging!
 *
 * @param types a bit-mask with all types (MM_CONT,MM_DEF) to use for counting
 * @return the number of free frames
 */
u32 mm_getFreeFrames(u32 types);

/**
 * Allocates <count> contiguous frames from the MM-bitmap
 *
 * @param count the number of frames
 * @param align the alignment of the memory (in pages)
 * @return the first allocated frame or negative if an error occurred
 */
s32 mm_allocateContiguous(u32 count,u32 align);

/**
 * Free's <count> contiguous frames, starting at <first> in the MM-bitmap
 *
 * @param first the first frame-number
 * @param count the number of frames
 */
void mm_freeContiguous(u32 first,u32 count);

/**
 * Allocates a frame and returns the frame-number
 *
 * @panic if there is no frame left anymore
 * @return the frame-number
 */
u32 mm_allocate(void);

/**
 * Frees the given frame. Note that you can free frames allocated with mm_allocate() and
 * mm_allocateContiguous() with this function!
 *
 * @param frame the frame-number
 */
void mm_free(u32 frame);

#if DEBUGGING

/**
 * Prints all free frames
 *
 * @param types a bit-mask with all types (MM_CONT,MM_DEF) to use for counting
 */
void mm_dbg_printFreeFrames(u32 types);

#endif

#endif /*MM_H_*/
