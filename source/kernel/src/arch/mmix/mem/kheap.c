/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>

uintptr_t kheap_allocAreas(void) {
	if(pmem_getFreeFrames(MM_DEF) < 1)
		return 0;
	return DIR_MAPPED_SPACE | (pmem_allocate() * PAGE_SIZE);
}

uintptr_t kheap_allocSpace(size_t count) {
	if(pmem_getFreeFrames(MM_CONT) < count)
		return 0;
	return DIR_MAPPED_SPACE | (pmem_allocateContiguous(count,1) * PAGE_SIZE);
}
