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

#pragma once

#include <sys/common.h>
#include <sys/spinlock.h>

#ifdef __i386__
#include <sys/arch/i586/mem/physmem.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/mem/physmem.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/mem/physmem.h>
#endif

/* converts bytes to pages */
#define BYTES_2_PAGES(b)		(((size_t)(b) + (PAGE_SIZE - 1)) >> PAGE_SIZE_SHIFT)

class Thread;
class OStream;

class PhysMem {
	PhysMem() = delete;

	struct SwapInJob {
		uintptr_t addr;
		Thread *thread;
		SwapInJob *next;
	};

	static const size_t BITS_PER_BMWORD				= sizeof(tBitmap) * 8;
	static const ulong KERNEL_MEM_PERCENT			= 20;
	static const ulong KERNEL_MEM_MIN				= 750;
	static const ulong MAX_SWAP_AT_ONCE				= 10;
	static const ulong SWAPIN_JOB_COUNT				= 64;
	/* the amount of memory at which we should start to set the region-timestamp */
	static const size_t REG_TS_BEGIN				= 1024 * PAGE_SIZE;

public:
	enum MemType {
		CONT	= 1,
		DEF		= 2
	};

	enum FrameType {
		CRIT,
		KERN,
		USR
	};

	/**
	 * Initializes the memory-management
	 */
	static void init();

	/**
	 * Checks whether its allowed to map the given physical address range
	 *
	 * @param addr the start-address
	 * @param size the size of the area
	 * @return true if so
	 */
	static bool canMap(uintptr_t addr,size_t size);

	/**
	 * @return whether we can swap
	 */
	static bool canSwap() {
		return swapEnabled;
	}

	/**
	 * @return the number of bytes used for the mm-stack
	 */
	static size_t getStackSize() {
		return stackPages * PAGE_SIZE;
	}

	/**
	 * Marks all memory available/occupied as necessary.
	 * This function should not be called by other modules!
	 */
	static void markAvailable();

	/**
	 * Counts the number of free frames. This is primarly intended for debugging!
	 *
	 * @param types a bit-mask with all types (PhysMem::CONT,PhysMem::DEF) to use for counting
	 * @return the number of free frames
	 */
	static size_t getFreeFrames(uint types);

	/**
	 * Determines whether the region-timestamp should be set (depending on the still available memory)
	 *
	 * @return true if so
	 */
	static bool shouldSetRegTimestamp();

	/**
	 * Allocates <count> contiguous frames from the MM-bitmap
	 *
	 * @param count the number of frames
	 * @param align the alignment of the memory (in pages)
	 * @return the first allocated frame or negative if an error occurred
	 */
	static ssize_t allocateContiguous(size_t count,size_t align);

	/**
	 * Free's <count> contiguous frames, starting at <first> in the MM-bitmap
	 *
	 * @param first the first frame-number
	 * @param count the number of frames
	 */
	static void freeContiguous(frameno_t first,size_t count);

	/**
	 * Starts the swapping-system. This HAS TO be done with the swapping-thread!
	 * Assumes that swapping is enabled.
	 */
	static void swapper();

	/**
	 * Swaps out frames until at least <frameCount> frames are available.
	 * Panics if its not possible to make that frames available (swapping disabled, partition full, ...)
	 *
	 * @param frameCount the number of frames you need
	 * @return true on success
	 */
	static bool reserve(size_t frameCount);

	/**
	 * Allocates one frame. Assumes that it is available. You should announce it with reserve()
	 * first!
	 *
	 * @param type the type of memory (FRM_*)
	 * @return the frame-number or 0 if no free frame is available
	 */
	static frameno_t allocate(FrameType type);

	/**
	 * Frees the given frame
	 *
	 * @param frame the frame-number
	 * @param type the type of memory (FRM_*)
	 */
	static void free(frameno_t frame,FrameType type);

	/**
	 * Swaps the page with given address for the current process in
	 *
	 * @param addr the address of the page
	 * @return true if successfull
	 */
	static bool swapIn(uintptr_t addr);

	/**
	 * Prints information about the pmem-module
	 *
	 * @param os the output-stream
	 */
	static void print(OStream &os);

	/**
	 * Prints the free frames on the stack
	 *
	 * @param os the output-stream
	 */
	static void printStack(OStream &os);

	/**
	 * Prints the free contiguous frames
	 *
	 * @param os the output-stream
	 */
	static void printCont(OStream &os);

private:
	/**
	 * Inits the architecture-specific part of the physical-memory management.
	 * This function should not be called by other modules!
	 *
	 * @param stackBegin is set to the beginning of the stack
	 * @param stackSize the size of the stack
	 * @param bitmap the start of the bitmap
	 */
	static void initArch(uintptr_t *stackBegin,size_t *stackSize,tBitmap **bitmap);

private:
	static size_t getFreeDef();
	static void markRangeUsed(uintptr_t from,uintptr_t to,bool used);
	static void doMarkRangeUsed(uintptr_t from,uintptr_t to,bool used);
	static void markUsed(frameno_t frame,bool used);
	static void appendJob(SwapInJob *job);
	static SwapInJob *getJob();
	static void freeJob(SwapInJob *job);

	/* the bitmap for the frames of the lowest few MB; 0 = free, 1 = used */
	static tBitmap *bitmap;
	static uintptr_t bitmapStart;
	static size_t freeCont;

	/* We use a stack for the remaining memory
	 * TODO Currently we don't free the frames for the stack */
	static size_t stackPages;
	static uintptr_t stackBegin;
	static frameno_t *stack;
	static klock_t contLock;
	static klock_t defLock;
	static bool initialized;

	/* for swapping */
	static size_t swappedOut;
	static size_t swappedIn;
	static bool swapEnabled;
	static bool swapping;
	static Thread *swapperThread;
	static size_t cframes;	/* critical frames; for dynarea, cache and heap */
	static size_t kframes;	/* kernel frames: for pagedirs, page-tables, kstacks, ... */
	static size_t uframes;	/* user frames */
	/* swap-in jobs */
	static SwapInJob siJobs[];
	static SwapInJob *siFreelist;
	static SwapInJob *siJobList;
	static SwapInJob *siJobEnd;
	static size_t jobWaiters;
};

inline bool PhysMem::shouldSetRegTimestamp() {
	bool res;
	if(!swapEnabled)
		return false;
	SpinLock::acquire(&defLock);
	res = getFreeDef() * PAGE_SIZE < REG_TS_BEGIN;
	SpinLock::release(&defLock);
	return res;
}
