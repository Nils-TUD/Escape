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
#include <assert.h>

class Cache;

class KHeap {
	friend class Cache;

	KHeap() = delete;

	struct MemArea {
		size_t size;
		void *address;
		MemArea *next;
	};

	/* the number of entries in the occupied map */
	static const size_t OCC_MAP_SIZE			= 1024;
	static const ulong GUARD_MAGIC				= 0xDEADBEEF;

public:
	/**
	 * Allocates <size> bytes in kernel-space and returns the pointer to the beginning of
	 * the allocated memory. If there is not enough memory the function returns NULL.
	 *
	 * @param size the number of bytes to allocate
	 * @return the address of the memory or NULL
	 */
	static void *alloc(size_t size);

	/**
	 * Allocates space for <num> elements, each <size> big, on the heap and memset's the area to 0.
	 * If there is not enough memory the function returns NULL.
	 *
	 * @param num the number of elements
	 * @param size the size of each element
	 * @return the address of the memory or NULL
	 */
	static void *calloc(size_t num,size_t size);

	/**
	 * Reallocates the area at given address to the given size. That means either your data will
	 * be copied to a different address or your area will be resized.
	 *
	 * @param addr the address of your area
	 * @param size the number of bytes your area should be resized to
	 * @return the address (may be different) of your area or NULL if there is not enough mem
	 */
	static void *realloc(void *addr,size_t size);

	/**
	 * Frees the via kmalloc() allocated area starting at <addr>.
	 */
	static void free(void *addr);

	/**
	 * @return the number of allocated pages
	 */
	static size_t getPageCount() {
		return pages;
	}

	/**
	 * @return the number of used bytes
	 */
	static size_t getUsedMem();

	/**
	 * @return the total number of bytes occupied (frames reserved; maybe not all in use atm)
	 */
	static size_t getOccupiedMem() {
		return memUsage;
	}

	/**
	 * Note that this function is intended for debugging-purposes only!
	 *
	 * @return the number of free bytes
	 */
	static size_t getFreeMem();

	/**
	 * Prints the kernel-heap data-structure
	 */
	static void print();

private:
	/**
	 * Internal: Allocates one page for areas
	 *
	 * @return the address at which the space can be accessed
	 */
	static uintptr_t allocAreas();

	/**
	 * Internal: Allocates <count> pages for data
	 *
	 * @param count the number of pages
	 * @return the address at which the space can be accessed
	 */
	static uintptr_t allocSpace(size_t count);

	/**
	 * Adds the given memory-range to the heap as free space
	 *
	 * @param addr the start-address
	 * @param size the size
	 * @return true if the memory has been added (we need areas to do so, which might not be available
	 * 	when no area is free anymore and no free frame is available)
	 */
	static bool addMemory(uintptr_t addr,size_t size);

	static bool loadNewAreas(void);
	static bool doAddMemory(uintptr_t addr,size_t size);
	static bool loadNewSpace(size_t size);
	static size_t getHash(void *addr);

	/* a linked list of free and usable areas. That means the areas have an address and size */
	static MemArea *usableList;
	/* a linked list of free but not usable areas. That means the areas have no address and size */
	static MemArea *freeList;
	/* a hashmap with occupied-lists, key is getHash(address) */
	static MemArea *occupiedMap[];
	/* currently occupied memory */
	static size_t memUsage;
	static size_t pages;
	static klock_t lock;
};

inline bool KHeap::addMemory(uintptr_t addr,size_t size) {
	bool res;
	SpinLock::aquire(&lock);
	res = doAddMemory(addr,size);
	SpinLock::release(&lock);
	return res;
}
