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

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/mem/cache.h>
#include <sys/mem/physmem.h>
#include <sys/mem/dynarray.h>
#include <sys/task/proc.h>
#include <esc/test.h>
#include "testutils.h"

static size_t mappedPages;
static size_t heapUsed,heapPages;
static size_t cacheUsed,cachePages;
static size_t daPages;
static size_t freeFrames;

void checkMemoryBefore(bool checkMappedPages) {
	if(checkMappedPages)
		mappedPages = Proc::getCurPageDir()->getPageCount();
	heapPages = KHeap::getPageCount();
	heapUsed = KHeap::getUsedMem();
	cachePages = Cache::getPageCount();
	cacheUsed = Cache::getUsedMem();
	daPages = DynArray::getTotalPages();
	freeFrames = PhysMem::getFreeFrames(PhysMem::DEF | PhysMem::CONT);
}

void checkMemoryAfter(bool checkMappedPages) {
	size_t newHeapUsed,newHeapPages;
	size_t newCacheUsed,newCachePages;
	size_t newFreeFrames;
	size_t newDaPages;
	newHeapPages = KHeap::getPageCount();
	newHeapUsed = KHeap::getUsedMem();
	newCachePages = Cache::getPageCount();
	newCacheUsed = Cache::getUsedMem();
	newDaPages = DynArray::getTotalPages();
	newFreeFrames = PhysMem::getFreeFrames(PhysMem::DEF | PhysMem::CONT);
	if(checkMappedPages) {
		size_t newMappedPages = Proc::getCurPageDir()->getPageCount();
		test_assertSize(newMappedPages,mappedPages);
	}
	test_assertSize(newHeapUsed,heapUsed);
	test_assertSize(newCacheUsed,cacheUsed);
	/* the heap, cache and the dynamic-arrays might have been extended; this is not considered
	 * as a memory-leak, since they are not free'd anyway. */
	test_assertSize(
		newFreeFrames +
			((newDaPages + newHeapPages + newCachePages) - (daPages + heapPages + cachePages)),
		freeFrames
	);
}
