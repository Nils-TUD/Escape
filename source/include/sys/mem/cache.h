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
#include <sys/mem/paging.h>

class Cache {
	Cache() = delete;

	struct Entry {
		const size_t objSize;
		size_t totalObjs;
		size_t freeObjs;
		void *freeList;
	};

public:
	/**
	 * Allocates <size> bytes from the cache
	 *
	 * @param size the size
	 * @return the pointer to the allocated area or NULL
	 */
	static void *alloc(size_t size) asm("cache_alloc");

	/**
	 * Allocates <num> objects with <size> bytes from the cache and zero's the memory.
	 *
	 * @param num the number of objects
	 * @param size the size of an object
	 * @return the pointer to the allocated area or NULL
	 */
	static void *calloc(size_t num,size_t size) asm("cache_calloc");

	/**
	 * Reallocates the area at <p> to be <size> bytes large. This may involve moving it to a different
	 * location.
	 *
	 * @param p the area
	 * @param size the new size of the area
	 * @return the potentially new location of the area or NULL
	 */
	static void *realloc(void *p,size_t size) asm("cache_realloc");

	/**
	 * Frees the given area
	 *
	 * @param p the area
	 */
	static void free(void *p) asm("cache_free");

	/**
	 * @return the number of pages used by the cache
	 */
	static size_t getPageCount() {
		return pages;
	}

	/**
	 * @return the occupied memory
	 */
	static size_t getOccMem();

	/**
	 * @return the used memory
	 */
	static size_t getUsedMem();

	/**
	 * Prints the cache
	 */
	static void print();

	#if DEBUGGING
	/**
	 * Enables/disables "allocate and free" prints
	 *
	 * @param enabled the new value
	 */
	static void dbg_setAaFEnabled(bool enabled) {
		aafEnabled = enabled;
	}
	#endif

private:
	static void printBar(size_t mem,size_t maxMem,size_t total,size_t free);
	static void *get(Entry *c,size_t i);

#if DEBUGGING
	static bool aafEnabled;
#endif
	static klock_t cacheLock;
	static size_t pages;
	static Entry caches[];
};
