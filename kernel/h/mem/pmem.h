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

#include <common.h>
#include <multiboot.h>

#define DEBUG_FRAME_USAGE		0

/* the physical start-address of the kernel-area */
#define KERNEL_AREA_P_ADDR		0x0
/* the physical start-address of the kernel itself */
#define KERNEL_P_ADDR			(1 * M)

/* total mem size (in bytes) */
#define MEMSIZE					(mb->memUpper * K - KERNEL_P_ADDR)

#define PAGE_SIZE				(4 * K)
#define PAGE_SIZE_SHIFT			12
#define L16M_PAGE_COUNT			((16 * M - KERNEL_P_ADDR) / PAGE_SIZE)
#define U16M_PAGE_COUNT			((MEMSIZE / PAGE_SIZE) - L16M_PAGE_COUNT)

/* converts bytes to pages */
#define BYTES_2_PAGES(b)		(((u32)(b) + (PAGE_SIZE - 1)) >> PAGE_SIZE_SHIFT)

#define L16M_CACHE_SIZE			16

/* set values to support bit-masks of the types */
typedef enum {MM_DMA = 1,MM_DEF = 2} eMemType;

/**
 * Initializes the memory-management
 */
void mm_init(void);

/**
 * Counts the number of free frames. This is primarly intended for debugging!
 *
 * @param types a bit-mask with all types (MM_DMA,MM_DEF) to use for counting
 * @return the number of free frames
 */
u32 mm_getFreeFrmCount(u32 types);

/**
 * A convenience-method to allocate multiple frames. Simply calls <count> times
 * mm_allocateFrame(<type>) and stores the frames in <frames>.
 *
 * @param type the frame-type: MM_DMA or MM_DEF
 * @param frames the array for <count> frames
 * @param count the number of frames you want to get
 */
void mm_allocateFrames(eMemType type,u32 *frames,u32 count);

/**
 * Allocates a frame of the given type and returns the frame-number
 *
 * @panic if there is no frame left anymore
 *
 * @param type the frame-type: MM_DMA or MM_DEF
 * @return the frame-number
 */
u32 mm_allocateFrame(eMemType type);

/**
 * A convenience-method to free multiple frames. Simply calls <count> times
 * mm_freeFrame(<frame>,<type>).
 *
 * @param type the frame-type: MM_DMA or MM_DEF
 * @param frames the array with <count> frames
 * @param count the number of frames you want to free
 */
void mm_freeFrames(eMemType type,u32 *frames,u32 count);

/**
 * Frees the given frame of the given type
 *
 * @param frame the frame-number
 * @param type the frame-type: MM_DMA or MM_DEF
 */
void mm_freeFrame(u32 frame,eMemType type);

#if DEBUGGING

#if DEBUG_FRAME_USAGE
void mm_dbg_printFrameUsage(void);
#endif

/**
 * Prints all free frames
 */
void mm_dbg_printFreeFrames(void);

#endif

#endif /*MM_H_*/
