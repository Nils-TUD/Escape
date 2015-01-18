/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <mem/dynarray.h>
#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <common.h>
#include <string.h>
#include <util.h>

bool DynArray::extend() {
	Region *reg;

	/* region full? */
	if(areaBegin + objCount * objSize + PAGE_SIZE > areaBegin + areaSize)
		return false;

	reg = regions;
	if(reg == NULL) {
		reg = regions = freeList;
		if(reg == NULL)
			return false;
		freeList = freeList->next;
		reg->addr = areaBegin;
		reg->size = 0;
		reg->next = NULL;
	}

	uintptr_t addr = reg->addr + reg->size;
	PageTables::KAllocator alloc;
	if(PageDir::mapToCur(addr,1,alloc,PG_SUPERVISOR | PG_WRITABLE | PG_PRESENT) < 0) {
		reg->next = freeList;
		freeList = reg;
		return false;
	}
	memclear((void*)addr,PAGE_SIZE);
	totalPages++;
	reg->size += PAGE_SIZE;
	objCount = reg->size / objSize;
	return true;
}

DynArray::~DynArray() {
	if(regions) {
		PageTables::KAllocator alloc;
		PageDir::unmapFromCur(regions->addr,regions->size / PAGE_SIZE,alloc);
		totalPages -= regions->size / PAGE_SIZE;
		/* put region on freelist */
		regions->next = freeList;
		freeList = regions;
		regions = NULL;
	}
}
