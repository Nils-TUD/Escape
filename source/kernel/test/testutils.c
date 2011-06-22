/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/mem/cache.h>
#include <sys/mem/pmem.h>
#include <sys/mem/dynarray.h>
#include <esc/test.h>
#include "testutils.h"

static size_t mappedPages;
static size_t heapUsed,heapPages;
static size_t cacheUsed,cachePages;
static size_t daPages;
static size_t freeFrames;

void checkMemoryBefore(bool checkMappedPages) {
	if(checkMappedPages)
		mappedPages = paging_dbg_getPageCount();
	heapPages = kheap_getPageCount();
	heapUsed = kheap_getUsedMem();
	cachePages = cache_getPageCount();
	cacheUsed = cache_getUsedMem();
	daPages = dyna_getTotalPages();
	freeFrames = pmem_getFreeFrames(MM_DEF | MM_CONT);
}

void checkMemoryAfter(bool checkMappedPages) {
	size_t newHeapUsed,newHeapPages;
	size_t newCacheUsed,newCachePages;
	size_t newFreeFrames;
	size_t newDaPages;
	newHeapPages = kheap_getPageCount();
	newHeapUsed = kheap_getUsedMem();
	newCachePages = cache_getPageCount();
	newCacheUsed = cache_getUsedMem();
	newDaPages = dyna_getTotalPages();
	newFreeFrames = pmem_getFreeFrames(MM_DEF | MM_CONT);
	if(checkMappedPages) {
		size_t newMappedPages = paging_dbg_getPageCount();
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
