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
typedef enum {FRM_CRIT,FRM_KERNEL,FRM_USER} eFrmType;

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
 * Checks whether its allowed to map the given physical address range
 *
 * @param addr the start-address
 * @param size the size of the area
 * @return true if so
 */
bool pmem_canMap(uintptr_t addr,size_t size);

/**
 * @return whether we can swap
 */
bool pmem_canSwap(void);

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
 * Determines whether the region-timestamp should be set (depending on the still available memory)
 *
 * @return true if so
 */
bool pmem_shouldSetRegTimestamp(void);

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
 * Starts the swapping-system. This HAS TO be done with the swapping-thread!
 * Assumes that swapping is enabled.
 */
void pmem_swapper(void);

/**
 * Swaps out frames until at least <frameCount> frames are available.
 * Panics if its not possible to make that frames available (swapping disabled, partition full, ...)
 *
 * @param frameCount the number of frames you need
 * @return true on success
 */
bool pmem_reserve(size_t frameCount);

/**
 * Allocates one frame. Assumes that it is available. You should announce it with pmem_reserve()
 * first!
 *
 * @param type the type of memory (FRM_*)
 * @return the frame-number or 0 if no free frame is available
 */
frameno_t pmem_allocate(eFrmType type);

/**
 * Frees the given frame
 *
 * @param frame the frame-number
 * @param type the type of memory (FRM_*)
 */
void pmem_free(frameno_t frame,eFrmType type);

/**
 * Swaps the page with given address for the current process in
 *
 * @param addr the address of the page
 * @return true if successfull
 */
bool pmem_swapIn(uintptr_t addr);

/**
 * Prints information about the pmem-module
 */
void pmem_print(void);

/**
 * Prints the free frames on the stack
 */
void pmem_printStack(void);

/**
 * Prints the free contiguous frames
 */
void pmem_printCont(void);

#endif /*PMEM_H_*/
