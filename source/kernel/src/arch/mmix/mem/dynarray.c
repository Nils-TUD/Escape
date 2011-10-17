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

#define DYNA_REG_COUNT	128

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
	frameno_t frame;
	sDynaRegion *reg;

	/* we use the begin for the current size here */
	if(d->regions == NULL)
		d->areaBegin = 0;

	/* region full? */
	if(d->areaBegin + PAGE_SIZE > d->areaSize)
		return false;

	/* allocate new region */
	reg = freeList;
	if(reg == NULL)
		return false;

	/* allocate a frame */
	frame = pmem_allocate(true);
	if(frame == 0)
		return false;

	/* remove from freelist */
	freeList = freeList->next;
	reg->next = NULL;
	/* append to regions */
	if(d->regions == NULL)
		d->regions = reg;
	else {
		sDynaRegion *r = d->regions;
		while(r->next != NULL)
			r = r->next;
		r->next = reg;
	}

	reg->addr = DIR_MAPPED_SPACE | (frame * PAGE_SIZE);
	reg->size = PAGE_SIZE;
	totalPages++;
	/* clear it and increase total size and number of objects */
	memclear((void*)reg->addr,PAGE_SIZE);
	d->areaBegin += PAGE_SIZE;
	d->objCount += PAGE_SIZE / d->objSize;
	return true;
}

void dyna_destroy(sDynArray *d) {
	sDynaRegion *reg = d->regions;
	while(reg != NULL) {
		sDynaRegion *next = reg->next;
		pmem_free((reg->addr & ~DIR_MAPPED_SPACE) / PAGE_SIZE,true);
		totalPages--;
		/* put region on freelist */
		reg->next = freeList;
		freeList = reg;
		reg = next;
	}
	d->regions = NULL;
}
