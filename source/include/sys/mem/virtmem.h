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
#include <sys/mem/vmtree.h>
#include <sys/mem/vmfreemap.h>
#include <sys/mem/paging.h>
#include <sys/col/slist.h>

#ifdef DEBUGGING
#	define DISABLE_DEMLOAD	1
#else
#	define DISABLE_DEMLOAD	0
#endif

#define MAX_REGUSE_COUNT	8192

#define RNO_TEXT			0
#define RNO_RODATA			1
#define RNO_DATA			2

#define PROT_READ			0
#define PROT_WRITE			RF_WRITABLE
#define PROT_EXEC			RF_EXECUTABLE

#define MAP_SHARED			RF_SHAREABLE
#define MAP_PRIVATE			0
#define MAP_STACK			RF_STACK
#define MAP_GROWABLE		RF_GROWABLE
#define MAP_GROWSDOWN		RF_GROWS_DOWN
#define MAP_NOFREE			RF_NOFREE
#define MAP_TLS				RF_TLS
#define MAP_POPULATE		256UL
#define MAP_NOMAP			512UL
#define MAP_FIXED			1024UL

class Proc;
class Thread;
class OStream;
class Region;

class VirtMem : public SListItem {
	friend class ProcBase;

public:
	/**
	 * Tries to handle a page-fault for the given address. That means, loads a page on demand, zeros
	 * it on demand, handles copy-on-write or swapping.
	 *
	 * @param addr the address that caused the page-fault
	 * @param write whether its a write-access
	 * @return true if successfull
	 */
	static bool pagefault(uintptr_t addr,bool write);

	/**
	 * Swaps <count> pages out. It searches for the best suited page to swap out.
	 *
	 * @param pid the process-id for writing the page-content to <file>
	 * @param file the file to write to
	 * @param count the number of pages to swap out
	 */
	static void swapOut(pid_t pid,OpenFile *file,size_t count);

	/**
	 * Swaps the page at given address of the given process in.
	 *
	 * @param pid the process-id for writing the page-content to <file>
	 * @param file the file to write to
	 * @param t the thread that wants to swap the page in (and has reserved the frame to do so)
	 * @param addr the address of the page to swap in
	 * @return true on success
	 */
	static bool swapIn(pid_t pid,OpenFile *file,Thread *t,uintptr_t addr);

	/**
	 * Sets the timestamp for all regions that are used by the given thread
	 *
	 * @param t the thread
	 * @param timestamp the timestamp to set
	 */
	static void setTimestamp(Thread *t,uint64_t timestamp);

	/**
	 * @return the id of the process that owns this virtual memory
	 */
	pid_t getPid() const {
		return pid;
	}
	/**
	 * @return the page-directory
	 */
	PageDir *getPageDir() const {
		// TODO is that necessary?
		return const_cast<PageDir*>(&pagedir);
	}
	/**
	 * @return the number of own frames
	 */
	size_t getOwnFrames() const {
		return ownFrames;
	}
	/**
	 * @return the number of shared frames
	 */
	size_t getSharedFrames() const {
		return sharedFrames;
	}
	/**
	 * @return the number of swapped out frames
	 */
	size_t getSwappedFrames() const {
		return swapped;
	}

	/**
	 * @return stats
	 */
	size_t getPeakOwnFrames() const {
		return peakOwnFrames;
	}
	size_t getPeakSharedFrames() const {
		return peakSharedFrames;
	}
	size_t getSwapCount() const {
		return swapCount;
	}

	/**
	 * Adds a region for physical memory mapped into the virtual memory (e.g. for vga text-mode or DMA).
	 * Please use this function instead of add() because this one maps the pages!
	 *
	 * @param phys a pointer to the physical memory to map; if *phys is 0, the function allocates
	 * 	contiguous physical memory itself and stores the address in *phys
	 * @param bCount the number of bytes to map
	 * @param align the alignment for the allocated physical-memory (just if *phys = 0)
	 * @param writable whether it should be mapped writable
	 * @return the virtual address or 0 if failed
	 */
	uintptr_t addPhys(uintptr_t *phys,size_t bCount,size_t align,bool writable);

	/**
	 * Maps a region to this VM.
	 *
	 * @param addr the virtual address of the region (ignored if MAP_FIXED is not set in <flags>)
	 * @param length the number of bytes
	 * @param loadCount the number of bytes to demand-load
	 * @param prot the protection flags (PROT_*)
	 * @param flags the map flags (MAP_*)
	 * @param f the file to map (may be NULL)
	 * @param offset the offset in the binary from where to load the region (ignored if bin is NULL)
	 * @param vmreg will be set to the created region
	 * @return 0 on success or a negative error-code
	 */
	int map(uintptr_t addr,size_t length,size_t loadCount,int prot,int flags,OpenFile *f,
			off_t offset,VMRegion **vmreg);

	/**
	 * Changes the protection-settings of the region @ <addr>. This is not possible for TLS-, stack-
	 * and nofree-regions.
	 *
	 * @param addr the virtual address
	 * @param flags the new flags (RF_WRITABLE or 0)
	 * @return 0 on success
	 */
	int regctrl(uintptr_t addr,ulong flags);

	/**
	 * This is a helper-function for determining the real memory-usage of all processes. It counts
	 * the number of present frames in all regions of the given process and divides them for each
	 * region by the number of region-users. It does not count cow-pages!
	 * This way, we don't count shared regions multiple times (at the end the division sums up to
	 * one usage of the region).
	 *
	 * @param pages will point to the number of pages (size of virtual-memory)
	 * @return the number of used frames for this process
	 */
	float getMemUsage(size_t *pages) const;

	/**
	 * Gets the region at given address
	 *
	 * @param addr the address
	 * @return the region
	 */
	VMRegion *getRegion(uintptr_t addr) {
		return regtree.getByAddr(addr);
	}

	/**
	 * Queries the start- and end-address of a region
	 *
	 * @param reg the region-number
	 * @param start will be set to the start-address
	 * @param end will be set to the end-address (exclusive; i.e. 0x1000 means 0xfff is the last
	 *  accessible byte)
	 * @param locked whether to lock the regions of the given process during the operation
	 * @return true if the region exists
	 */
	bool getRegRange(VMRegion *vm,uintptr_t *start,uintptr_t *end,bool locked);

	/**
	 * Removes all regions, optionally including stack.
	 *
	 * @param remStack whether to remove the stack too
	 */
	void removeAll(bool remStack);

	/**
	 * Removes the given region
	 *
	 * @param vm the region
	 */
	void remove(VMRegion *vm);

	/**
	 * Joins virtmem <dst> to the region <rno> of this virtmem. This can only be used for shared-
	 * memory!
	 *
	 * @param rno the region-number in the source-process
	 * @param dst the destination-virtmem
	 * @param nvm the new created region
	 * @param dstVirt the virtual address where to create that region (0 = auto)
	 * @return 0 on success or the negative error-code
	 */
	int join(uintptr_t srcAddr,VirtMem *dst,VMRegion **nvm,uintptr_t dstVirt);

	/**
	 * Clones all regions of this virtmem (current) into the destination-virtmem
	 *
	 * @param dst the destination-virtmem
	 * @return 0 on success
	 */
	int cloneAll(VirtMem *dst);

	/**
	 * If <amount> is positive, the region will be grown by <amount> pages. If negative it
	 * will be shrinked. If 0 it returns the current offset to the region-beginning, in pages.
	 *
	 * @param addr the address of the region
	 * @param amount the number of pages to add/remove
	 * @return the old region-end or 0 if failed
	 */
	size_t grow(uintptr_t addr,ssize_t amount);

	/**
	 * Grows the stack of the given thread so that <addr> is accessible, if possible
	 *
	 * @param vm the region
	 * @param addr the address
	 * @return 0 on success
	 */
	int growStackTo(VMRegion *vm,uintptr_t addr);

	/**
	 * Convenience-method for the data-region.
	 *
	 * @param amount the number of pages to add/remove
	 * @return the old region-end or 0 if failed
	 */
	size_t growData(ssize_t amount) {
		return grow(dataAddr,amount);
	}

	/**
	 * Prints information about all mappings
	 *
	 * @param os the output-stream
	 */
	void printMaps(OStream &os) const;

	/**
	 * Prints a short version of the regions
	 *
	 * @param os the output-stream
	 * @param prefix the print prefix (e.g. tabs)
	 */
	void printShort(OStream &os,const char *prefix) const;

	/**
	 * Prints all regions
	 *
	 * @param os the output-stream
	 */
	void printRegions(OStream &os) const;

	/**
	 * Prints information about the virtmem
	 *
	 * @param os the output-stream
	 */
	void print(OStream &os) const;

private:
	/**
	 * Inits this object
	 *
	 * @param pid the pid to set
	 */
	void init(pid_t pid) {
		this->pid = pid;
		ownFrames = sharedFrames = swapped = 0;
		freeStackAddr = dataAddr = 0;
		peakOwnFrames = peakSharedFrames = swapCount = 0;
		VMFreeMap::init(&freemap,FREE_AREA_BEGIN,FREE_AREA_END - FREE_AREA_BEGIN);
		VMTree::addTree(this,&regtree);
	}

	/**
	 * Destroys this object
	 */
	void destroy() {
		freemap.destroy();
		VMTree::remTree(&regtree);
		pagedir.destroy();
	}

	/**
	 * Resets the stats
	 */
	void resetStats() {
		peakOwnFrames = ownFrames;
		peakSharedFrames = sharedFrames;
		swapCount = 0;
	}

	static Region *getLRURegion();
	static uintptr_t getBinary(OpenFile *file,VirtMem *&binOwner);
	static ssize_t getPgIdxForSwap(const Region *reg);
	static void setSwappedOut(Region *reg,size_t index);
	static void setSwappedIn(Region *reg,size_t index,frameno_t frameNo);

	bool doPagefault(uintptr_t addr,VMRegion *vm,bool write);
	void sync(VMRegion *vm) const;
	void doRemove(VMRegion *vm);
	size_t doGrow(VMRegion *vm,ssize_t amount);
	bool demandLoad(VMRegion *vm,uintptr_t addr);
	bool loadFromFile(VMRegion *vm,uintptr_t addr,size_t loadCount);
	uintptr_t findFreeStack(size_t byteCount,ulong rflags);
	VMRegion *isOccupied(uintptr_t start,uintptr_t end) const;
	uintptr_t getFirstUsableAddr() const;
	const char *getRegName(const VMRegion *vm) const;

	bool acquire() const;
	bool tryAquire() const;
	void release() const;

	void addOwn(long amount) {
		assert(amount > 0 || ownFrames >= (ulong)-amount);
		ownFrames += amount;
		peakOwnFrames = MAX(ownFrames,peakOwnFrames);
	}
	void addShared(long amount) {
		assert(amount > 0 || sharedFrames >= (ulong)-amount);
		sharedFrames += amount;
		peakSharedFrames = MAX(sharedFrames,peakSharedFrames);
	}
	void addSwap(long amount) {
		assert(amount > 0 || swapped >= (ulong)-amount);
		swapped += amount;
		swapCount += amount < 0 ? -amount : amount;
	}

	pid_t pid;
	/* the physical address for the page-directory of this process */
	PageDir pagedir;
	/* the number of frames the process owns, i.e. no cow, no shared stuff, no regaddphys.
	 * paging-structures are counted, too */
	ulong ownFrames;
	/* the number of frames the process uses, but maybe other processes as well */
	ulong sharedFrames;
	/* pages that are in swap */
	ulong swapped;
	/* for finding free positions more quickly: the start for the stacks */
	uintptr_t freeStackAddr;
	/* address of the data-region; required for chgsize */
	uintptr_t dataAddr;
	/* area-map for the free area */
	VMFreeMap freemap;
	/* the regions */
	VMTree regtree;
	/* mem stats */
	ulong peakOwnFrames;
	ulong peakSharedFrames;
	ulong swapCount;
};
