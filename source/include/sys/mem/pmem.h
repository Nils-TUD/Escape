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

#ifndef PMEM_H_
#define PMEM_H_

#include <sys/common.h>

#ifdef __i386__
#include <sys/arch/i586/mem/pmem.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/mem/pmem.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/mem/pmem.h>
#endif

/* converts bytes to pages */
#define BYTES_2_PAGES(b)		(((size_t)(b) + (PAGE_SIZE - 1)) >> PAGE_SIZE_SHIFT)

/* set values to support bit-masks of the types */
typedef enum {MM_CONT = 1,MM_DEF = 2} eMemType;

/**
 * Initializes the memory-management
 */
void pmem_init(void);

/**
 * Inits the architecture-specific part of the physical-memory management.
 * This function should not be called by other modules!
 *
 * @param stackBegin is set to the beginning of the stack
 * @param stackSize the size of the stack
 * @param bitmap the start of the bitmap
 */
void pmem_initArch(uintptr_t *stackBegin,size_t *stackSize,tBitmap **bitmap);

/**
 * Marks all memory available/occupied as necessary.
 * This function should not be called by other modules!
 */
void pmem_markAvailable(void);

/**
 * Marks the given range as used or not used.
 * This function should not be called by other modules!
 *
 * @param from the start-address
 * @param to the end-address
 * @param used whether the frame is used
 */
void pmem_markRangeUsed(uintptr_t from,uintptr_t to,bool used);

/**
 * @return the number of bytes used for the mm-stack
 */
size_t pmem_getStackSize(void);

/**
 * Counts the number of free frames. This is primarly intended for debugging!
 *
 * @param types a bit-mask with all types (MM_CONT,MM_DEF) to use for counting
 * @return the number of free frames
 */
size_t pmem_getFreeFrames(uint types);

/**
 * Allocates <count> contiguous frames from the MM-bitmap
 *
 * @param count the number of frames
 * @param align the alignment of the memory (in pages)
 * @return the first allocated frame or negative if an error occurred
 */
ssize_t pmem_allocateContiguous(size_t count,size_t align);

/**
 * Free's <count> contiguous frames, starting at <first> in the MM-bitmap
 *
 * @param first the first frame-number
 * @param count the number of frames
 */
void pmem_freeContiguous(frameno_t first,size_t count);

/**
 * Allocates a frame and returns the frame-number
 *
 * @panic if there is no frame left anymore
 * @return the frame-number
 */
frameno_t pmem_allocate(void);

/**
 * Frees the given frame. Note that you can free frames allocated with pmem_allocate() and
 * pmem_allocateContiguous() with this function!
 *
 * @param frame the frame-number
 */
void pmem_free(frameno_t frame);

/**
 * Prints all free frames
 *
 * @param types a bit-mask with all types (MM_CONT,MM_DEF) to use for counting
 */
void pmem_print(uint types);

#endif /*PMEM_H_*/
