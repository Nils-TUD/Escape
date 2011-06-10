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
	/* if its just one page, take a frame from the pmem-stack */
	if(count == 1)
		return kheap_allocAreas();
	/* otherwise we have to use contiguous physical memory */
	if(pmem_getFreeFrames(MM_CONT) < count)
		return 0;
	return DIR_MAPPED_SPACE | (pmem_allocateContiguous(count,1) * PAGE_SIZE);
}
