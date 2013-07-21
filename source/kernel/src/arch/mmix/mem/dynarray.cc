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
#include <sys/mem/physmem.h>
#include <sys/mem/dynarray.h>
#include <sys/util.h>
#include <string.h>

bool DynArray::extend() {
	frameno_t frame;
	Region *reg;

	/* we use the begin for the current size here */
	if(regions == NULL)
		areaBegin = 0;

	/* region full? */
	if(areaBegin + PAGE_SIZE > areaSize)
		return false;

	/* allocate new region */
	reg = freeList;
	if(reg == NULL)
		return false;

	/* allocate a frame */
	frame = PhysMem::allocate(PhysMem::CRIT);
	if(frame == 0)
		return false;

	/* remove from freelist */
	freeList = freeList->next;
	reg->next = NULL;
	/* append to regions */
	if(regions == NULL)
		regions = reg;
	else {
		Region *r = regions;
		while(r->next != NULL)
			r = r->next;
		r->next = reg;
	}

	reg->addr = DIR_MAPPED_SPACE | (frame * PAGE_SIZE);
	reg->size = PAGE_SIZE;
	totalPages++;
	/* clear it and increase total size and number of objects */
	memclear((void*)reg->addr,PAGE_SIZE);
	areaBegin += PAGE_SIZE;
	objCount += PAGE_SIZE / objSize;
	return true;
}

DynArray::~DynArray() {
	Region *reg = regions;
	while(reg != NULL) {
		Region *next = reg->next;
		PhysMem::free((reg->addr & ~DIR_MAPPED_SPACE) / PAGE_SIZE,PhysMem::CRIT);
		totalPages--;
		/* put region on freelist */
		reg->next = freeList;
		freeList = reg;
		reg = next;
	}
	regions = NULL;
}
