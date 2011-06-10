/**
 * $Id$
 */

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/dynarray.h>
#include <sys/util.h>
#include <string.h>

#define DYNA_REG_COUNT	128

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
	/* we use the begin for the current size here */
	if(d->_regions == NULL)
		d->_areaBegin = 0;

	/* region full? */
	if(d->_areaBegin + PAGE_SIZE > d->_areaSize)
		return false;

	/* allocate new region */
	sDynaRegion *reg = freeList;
	if(reg == NULL)
		util_panic("No free dynamic-array-regions");
	freeList = freeList->next;
	reg->next = NULL;

	/* append to regions */
	if(d->_regions == NULL)
		d->_regions = reg;
	else {
		sDynaRegion *r = d->_regions;
		while(r->next != NULL)
			r = r->next;
		r->next = reg;
	}

	/* allocate a frame */
	if(pmem_getFreeFrames(MM_DEF) == 0)
		return false;
	reg->addr = DIR_MAPPED_SPACE | (pmem_allocate() * PAGE_SIZE);
	reg->size = PAGE_SIZE;
	/* clear it and increase total size and number of objects */
	memclear((void*)reg->addr,PAGE_SIZE);
	d->_areaBegin += PAGE_SIZE;
	d->objCount = d->_areaBegin / d->objSize;
	return true;
}

void dyna_destroy(sDynArray *d) {
	sDynaRegion *reg = d->_regions;
	while(reg != NULL) {
		sDynaRegion *next = reg->next;
		pmem_free((reg->addr & ~DIR_MAPPED_SPACE) / PAGE_SIZE);
		/* put region on freelist */
		reg->next = freeList;
		freeList = reg;
		reg = next;
	}
	d->_regions = NULL;
}
