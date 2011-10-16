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
/* but one additional one for the unit-tests doesn't hurt */
#define DYNA_REG_COUNT	4

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
	sDynaRegion *reg;
	uintptr_t addr;
	klock_aquire(&d->lock);

	reg = d->regions;
	if(reg == NULL) {
		reg = d->regions = freeList;
		if(reg == NULL)
			util_panic("No free dynamic-array-regions");
		freeList = freeList->next;
		reg->addr = d->areaBegin;
		reg->size = 0;
		reg->next = NULL;
	}

	addr = reg->addr + reg->size;
	paging_map(addr,NULL,1,PG_SUPERVISOR | PG_WRITABLE | PG_PRESENT);
	memclear((void*)addr,PAGE_SIZE);
	totalPages++;
	reg->size += PAGE_SIZE;
	d->objCount = reg->size / d->objSize;
	klock_release(&d->lock);
	return true;
}

void dyna_destroy(sDynArray *d) {
	if(d->regions) {
		paging_unmap(d->regions->addr,d->regions->size / PAGE_SIZE,true);
		totalPages -= d->regions->size / PAGE_SIZE;
		/* put region on freelist */
		d->regions->next = freeList;
		freeList = d->regions;
		d->regions = NULL;
	}
}
