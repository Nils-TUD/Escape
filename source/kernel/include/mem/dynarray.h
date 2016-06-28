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
#include <cppsupport.h>
#include <spinlock.h>

/* This module is intended to provide a dynamically extending region. That means you have a limited
 * area in virtual-memory, but allocate the pages in it when needed. This way we still have a
 * "real" array, i.e. you can use the index-operator to get an object from that area and - most
 * important - can use the index as id to quickly get an object by id. This is of course much faster
 * than a hashmap or similar. So it combines the advantages of data-structures where each element
 * is allocated on the heap (space-efficient) and hashmaps, that provide a quick access by a key.
 * Of course there are also disadvantages:
 * - it is still limited in virtual-memory. but we have enough of it atm, so thats not really a
 *   problem.
 * - in this array may be multiple unused objects. which make an iteration over all objects
 *   a bit slower than e.g. a linked list.
 * - atm we don't free frames, even if no objects are in it anymore. The reason is that its not
 *   that easy to determine when a complete frame is unused. At least it would make this concept
 *   far more difficult. And I think, in "normal" usages of escape, this will never be a problem.
 *
 * Note that using the heap for that (with realloc) is a bad idea, because if anybody stores a
 * pointer to an object somewhere and the address changes because of a realloc, we have a problem...
 *
 * Atm this is just used for vfs-nodes and the GFT. So, at places where we need an indexable array
 * and where we usally need only a few objects, but neither want to have a low limit nor want to
 * have a huge static array.
 */

class DynArray : public CacheAllocatable {
	/* describes a dynarray-region */
	struct Region {
		uintptr_t addr;
		size_t size;
		Region *next;
	};

/* for i586 and eco32, we need only 3 regions; one for gft, one for vfs-nodes and one for sll-nodes */
/* but one additional one for the unit-tests doesn't hurt */
#if defined(__x86__)
	static const size_t REG_COUNT		= 4;
#elif defined(__eco32__)
	static const size_t REG_COUNT		= 4;
#elif defined(__mmix__)
	static const size_t REG_COUNT		= 128;
#endif

public:
	/**
	 * Initializes the dynamic-array-system
	 */
	static void init();

	/**
	 * @return the total number of pages used for all dynamic arrays
	 */
	static size_t getTotalPages() {
		return totalPages;
	}

	/**
	 * Creates the given dynamic array
	 *
	 * @param objSize the size of one object
	 * @param areaBegin the beginning of the area in virtual memory where the objects should be
	 * @param areaSize the size of that area
	 */
	explicit DynArray(size_t objSize,uintptr_t areaBegin,size_t areaSize)
		: lock(), objCount(), objSize(objSize), areaBegin(areaBegin), areaSize(areaSize),
		  regions() {
	}
	/**
	 * Destroys the dynamic array
	 */
	~DynArray();

	/**
	 * @return the size of one object
	 */
	size_t getObjSize() const {
		return objSize;
	}
	/**
	 * @return the current number of available objects
	 */
	size_t getObjCount() const {
		return objCount;
	}

	/**
	 * Retrieves an object from the given dynarray with given index
	 *
	 * @param d the dynamic array
	 * @param index the index of the object
	 * @return the object or NULL if out of range
	 */
	void *getObj(size_t index) const;

	/**
	 * Retrieves the index for the given object
	 *
	 * @param obj the object
	 * @return the index or -1 if not found
	 */
	ssize_t getIndex(const void *obj) const;

	/**
	 * Extends this array. That means, it allocates one page more and changes the number of
	 * available objects correspondingly (objCount).
	 *
	 * @return true if successfull
	 */
	bool extend();

private:
	mutable SpinLock lock;
	/* number of objects currently avaiable */
	size_t objCount;
	size_t objSize;
	/* the area in virtual memory; might not be used, depending on the architecture */
	uintptr_t areaBegin;
	size_t areaSize;
	/* the regions for this array */
	Region *regions;

	static Region regionstore[];
	static Region *freeList;
	static size_t totalPages;
};
