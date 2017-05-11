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

#include <common.h>
#include <lockguard.h>
#include <spinlock.h>

#if defined(__i586__)
#	include <arch/i586/mem/physmem.h>
#elif defined(__x86_64__)
#	include <arch/x86_64//mem/physmem.h>
#elif defined(__eco32__)
#	include <arch/eco32/mem/physmem.h>
#elif defined(__mmix__)
#	include <arch/mmix/mem/physmem.h>
#endif

/* converts bytes to pages */
#define BYTES_2_PAGES(b)		(((size_t)(b) + (PAGE_SIZE - 1)) >> PAGE_BITS)

class Thread;
class OStream;

typedef ulong tBitmap;

class PhysMem {
	PhysMem() = delete;

	struct SwapInJob {
		uintptr_t addr;
		Thread *thread;
		SwapInJob *next;
	};

	struct StackFrames {
		size_t pages;
		frameno_t *begin;
		frameno_t *frames;
	};

	static const size_t BITS_PER_BMWORD				= sizeof(tBitmap) * 8;
	static const ulong KERNEL_MEM_PERCENT			= 20;
	static const ulong KERNEL_MEM_MIN				= 750;
	static const ulong MAX_SWAP_AT_ONCE				= 10;
	static const ulong SWAPIN_JOB_COUNT				= 64;
	/* the amount of memory at which we should start to set the region-timestamp */
	static const size_t REG_TS_BEGIN				= 4 * 1024 * PAGE_SIZE;

	static const int OPEN_RETRIES					= 1000;

public:
	static const frameno_t INVALID_FRAME			= -1;

	enum MemType {
		CONT	= 1,
		DEF		= 2
	};

	enum FrameType {
		CRIT,
		KERN,
		USR
	};

	enum Attributes {
		MATTR_WC	= 1 << 0,
	};

	/**
	 * Initializes the memory-management
	 */
	static void init();

	/**
	 * @return the total amount of memory
	 */
	static size_t getTotal() {
		return totalMem;
	}

	/**
	 * Checks whether its allowed to map the given physical address range
	 *
	 * @param addr the start-address
	 * @param size the size of the area
	 * @return true if so
	 */
	static bool canMap(uintptr_t addr,size_t size);

	/**
	 * Sets the given attributes to the physical memory range <addr> .. <addr> + <size>.
	 *
	 * @param addr the physical address
	 * @param size the number of bytes
	 * @param attr the attributes (MATTR_*)
	 * @return 0 on success
	 */
	static int setAttributes(uintptr_t addr,size_t size,uint attr);

	/**
	 * @return the number of bytes used for the mm-stack
	 */
	static size_t getStackSize() {
		return (lower.pages + upper.pages) * PAGE_SIZE;
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
	 * @param swap whether to actually swap if necessary (or return an error)
	 * @return true on success
	 */
	static bool reserve(size_t frameCount,bool swap);

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
	 * @return 0 if successfull
	 */
	static int swapIn(uintptr_t addr);

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
	static uintptr_t bitmapStartFrame();
	static uintptr_t lowerStart();
	static uintptr_t lowerEnd();
	static frameno_t allocFrame(bool forceLower);
	static void freeFrame(frameno_t frame);
	static size_t getFreeDef();
	static void markRangeUsed(uintptr_t from,uintptr_t to,bool used);
	static void doMarkRangeUsed(uintptr_t from,uintptr_t to,bool used);
	static void markUsed(frameno_t frame,bool used);
	static void appendJob(SwapInJob *job);
	static SwapInJob *getJob();
	static void freeJob(SwapInJob *job);

	static size_t totalMem;

	/* the bitmap for the frames of the lowest few MB; 0 = free, 1 = used */
	static tBitmap *bitmap;
	static uintptr_t bitmapStart;
	static size_t freeCont;
	static SpinLock contLock;

	/* We use a stack for the remaining memory
	 * TODO Currently we don't free the frames for the stack */
	static StackFrames lower;
	static StackFrames upper;
	static SpinLock defLock;

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
	if(!swapEnabled)
		return false;

	LockGuard<SpinLock> g(&defLock);
	return getFreeDef() * PAGE_SIZE < REG_TS_BEGIN;
}
