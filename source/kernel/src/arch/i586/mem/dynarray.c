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
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/dynarray.h>
#include <sys/util.h>
#include <string.h>

/* for i586 and eco32, we need only 3 regions; one for gft, one for vfs-nodes and one for sll-nodes */
#define DYNA_REG_COUNT	3

static sDynaRegion regions[DYNA_REG_COUNT];
static sDynaRegion *freeList;
static size_t totalPages = 0;

void dyna_init(void) {
	size_t i;
	regions[0].next = NULL;
	freeList = regions;
	for(i = 1; i < DYNA_REG_COUNT; i++) {
		regions[i].next = freeList;
		freeList = regions + i;
	}
}

size_t dyna_getTotalPages(void) {
	return totalPages;
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
	totalPages++;
	reg->size += PAGE_SIZE;
	d->objCount = reg->size / d->objSize;
	return true;
}

void dyna_destroy(sDynArray *d) {
	if(d->_regions) {
		paging_unmap(d->_regions->addr,d->_regions->size / PAGE_SIZE,true);
		totalPages -= d->_regions->size / PAGE_SIZE;
		/* put region on freelist */
		d->_regions->next = freeList;
		freeList = d->_regions;
		d->_regions = NULL;
	}
}
