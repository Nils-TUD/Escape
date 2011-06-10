/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/dynarray.h>
#include <sys/util.h>
#include <string.h>

/* for i586 and eco32, we need only 3 regions; one for gft, one for vfs-nodes and one for sll-nodes */
#define DYNA_REG_COUNT	3

static sDynaRegion regions[DYNA_REG_COUNT];
static sDynaRegion *freeList;

void dyna_init(void) {
	size_t i;
	regions[0].next = NULL;
	freeList = regions;
	for(i = 1; i < DYNA_REG_COUNT; i++) {
		regions[i].next = freeList;
		freeList = regions + i;
	}
}

bool dyna_extend(sDynArray *d) {
	sDynaRegion *reg = d->_regions;
	if(reg == NULL) {
		reg = d->_regions = freeList;
		if(reg == NULL)
			util_panic("No free dynamic-array-regions");
		freeList = freeList->next;
		reg->addr = d->_areaBegin;
		reg->size = 0;
		reg->next = NULL;
	}

	uintptr_t addr = reg->addr + reg->size;
	if(pmem_getFreeFrames(MM_DEF) == 0 || reg->size >= d->_areaSize)
		return false;
	paging_map(addr,NULL,1,PG_SUPERVISOR | PG_WRITABLE | PG_PRESENT);
	memclear((void*)addr,PAGE_SIZE);
	reg->size += PAGE_SIZE;
	d->objCount = reg->size / d->objSize;
	return true;
}

void dyna_destroy(sDynArray *d) {
	if(d->_regions) {
		paging_unmap(d->_regions->addr,d->_regions->size / PAGE_SIZE,true);
		/* put region on freelist */
		d->_regions->next = freeList;
		freeList = d->_regions;
		d->_regions = NULL;
	}
}
