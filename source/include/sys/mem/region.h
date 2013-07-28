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
#include <sys/printf.h>
#include <esc/sllist.h>
#include <sys/mutex.h>
#include <assert.h>

#define PF_BITCOUNT			3		/* number of bits occupied by real flags */
#define PF_COPYONWRITE		1UL
#define PF_DEMANDLOAD		2UL
#define PF_SWAPPED			4UL

#define RF_GROWABLE			1UL
#define RF_SHAREABLE		2UL
#define RF_WRITABLE			4UL
#define RF_EXECUTABLE		8UL
#define RF_STACK			16UL
#define RF_NOFREE			32UL	/* means that the memory should not be free'd on release */
#define RF_TLS				64UL	/* needed to distinguish TLS-regions from others */
#define RF_GROWS_DOWN		128UL

class Region {
	Region() = delete;

public:
	/**
	 * Creates a new region with given attributes. Initially the process-collection that use the
	 * region will be empty!
	 *
	 * @param file the file
	 * @param bCount the number of bytes
	 * @param lCount the number of bytes to load from file
	 * @param offset the offset in the binary (ignored if bin is NULL)
	 * @param pgFlags the flags of the pages (PF_*)
	 * @param flags the flags of the region (RF_*)
	 * @return the region or NULL if failed
	 */
	static Region *create(sFile *file,size_t bCount,size_t lCount,size_t offset,ulong pgFlags,ulong flags);

	/**
	 * Clones this region for the given process. That means it copies the attributes from the
	 * given region into a new one and puts <p> as the only user into it. It assumes that <reg>
	 * has just one user, too (since shared regions can't be cloned).
	 * The page-flags are simply copied, i.e. you have to handle copy-on-write!
	 *
	 * @param p the process
	 * @return the created region or NULL if failed
	 */
	Region *clone(const void *p) const;

	/**
	 * Destroys this region (regardless of the number of users!)
	 */
	void destroy();

	/**
	 * Aquires the lock for this region
	 */
	void acquire() {
		mutex_acquire(&lock);
	}
	/**
	 * Tries to acquire the lock for this region
	 *
	 * @return true if successfull
	 */
	bool tryAquire() {
		return mutex_tryAquire(&lock);
	}
	/**
	 * Releases the lock for this region
	 */
	void release() {
		mutex_release(&lock);
	}

	/**
	 * @return the flags that specify the attributes of this region
	 */
	ulong getFlags() const {
		return flags;
	}
	void setFlags(ulong flags) {
		this->flags = flags;
	}
	/**
	 * @return the file or NULL if the region is not backed by a file
	 */
	sFile *getFile() const {
		return file;
	}
	/**
	 * @return the offset in the binary
	 */
	off_t getOffset() const {
		return offset;
	}
	/**
	 * @return the number of bytes to load from file
	 */
	size_t getLoadCount() const {
		return loadCount;
	}
	/**
	 * @return the number of bytes
	 */
	size_t getByteCount() const {
		return byteCount;
	}
	/**
	 * @return the number of pages
	 */
	size_t getPageCount() const {
		return pfSize;
	}
	/**
	 * @return the timestamp of last usage (for swapping)
	 */
	uint64_t getTimestamp() const {
		return timestamp;
	}
	void setTimestamp(uint64_t ts) {
		timestamp = ts;
	}

	/**
	 * @return the flags of the given page
	 */
	ulong getPageFlags(size_t page) const {
		return pageFlags[page];
	}
	void setPageFlags(size_t page,ulong flags) {
		pageFlags[page] = flags;
	}

	/**
	 * @return the linked list of virtmem objects that use this region
	 */
	const sSLList *getVMList() const {
		return &vms;
	}

	/**
	 * Counts the number of present and swapped out pages in the region
	 *
	 * @param swapped will be set to the number of swapped out pages
	 * @return the number of present pages
	 */
	size_t pageCount(size_t *swapped) const;

	/**
	 * @return the number of references of the region
	 */
	size_t refCount() const;

	/**
	 * Returns the swap-block in which the page with given index is stored
	 *
	 * @param pageIndex the index of the page in the region
	 * @return the swap-block
	 */
	ulong getSwapBlock(size_t pageIndex) const;

	/**
	 * Sets the swap-block in which the page with given index is stored to <swapBlock>.
	 *
	 * @param pageIndex the index of the page in the region
	 * @param swapBlock the swap-block
	 */
	void setSwapBlock(size_t pageIndex,ulong swapBlock);

	/**
	 * Adds the given process as user to the region
	 *
	 * @param p the process
	 * @return true if successfull
	 */
	bool addTo(const void *p);

	/**
	 * Removes the given process as user from the region
	 *
	 * @param p the process
	 * @return true if found
	 */
	bool remFrom(const void *p);

	/**
	 * Grows/shrinks the region by <amount> pages. The added page-flags are always 0.
	 * Note that the stack grows downwards, all other regions upwards.
	 *
	 * @param amount the number of pages
	 * @return the number of free'd swapped pages or a negative error-code
	 */
	ssize_t grow(ssize_t amount);

	/**
	 * Prints information about the given region in the given buffer
	 *
	 * @param buf the buffer
	 * @param reg the region
	 * @param virt the virtual-address at which the region is mapped
	 */
	void sprintf(sStringBuffer *buf,uintptr_t virt) const;

	/**
	 * Prints the region
	 *
	 * @param virt the virtual-address at which the region is mapped
	 */
	void print(uintptr_t virt) const;

	/**
	 * Prints the flags of the region
	 */
	void printFlags() const;

	/**
	 * Prints the flags of the region into the given buffer
	 *
	 * @param buf the buffer
	 */
	void sprintfFlags(sStringBuffer *buf) const;

private:
	ulong flags;
	sFile *file;
	off_t offset;
	size_t loadCount;
	size_t byteCount;
	uint64_t timestamp;
	size_t pfSize;			/* size of pageFlags */
	ulong *pageFlags;		/* flags for each page; upper bits: swap-block, if swapped */
	sSLList vms;
	mutex_t lock;			/* lock for the procs-field (all others can't change or belong to
	 	 	 	 	 	 	   exactly 1 process, which is locked anyway) */
};

inline ulong Region::getSwapBlock(size_t pageIndex) const {
	assert(pageFlags[pageIndex] & PF_SWAPPED);
	return pageFlags[pageIndex] >> PF_BITCOUNT;
}

inline void Region::setSwapBlock(size_t pageIndex,ulong swapBlock) {
	pageFlags[pageIndex] &= (1 << PF_BITCOUNT) - 1;
	pageFlags[pageIndex] |= swapBlock << PF_BITCOUNT;
}

inline size_t Region::refCount() const {
	return sll_length(&vms);
}

inline bool Region::addTo(const void *p) {
	assert(sll_length(&vms) == 0 || (flags & RF_SHAREABLE));
	return sll_append(&vms,(void*)p);
}

inline bool Region::remFrom(const void *p) {
	return sll_removeFirstWith(&vms,(void*)p) != -1;
}
