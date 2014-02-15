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

#include <sys/common.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/physmem.h>
#include <sys/mem/dynarray.h>
#include <sys/util.h>
#include <string.h>

bool DynArray::extend() {
	lock.down();

	/* region full? */
	if(areaBegin + objCount * objSize + PAGE_SIZE > areaBegin + areaSize) {
		lock.up();
		return false;
	}

	Region *reg = regions;
	if(reg == NULL) {
		reg = regions = freeList;
		if(reg == NULL) {
			lock.up();
			return false;
		}
		freeList = freeList->next;
		reg->addr = areaBegin;
		reg->size = 0;
		reg->next = NULL;
	}

	uintptr_t addr = reg->addr + reg->size;
	if(PageDir::mapToCur(addr,NULL,1,PG_SUPERVISOR | PG_WRITABLE | PG_PRESENT) < 0) {
		reg->next = freeList;
		freeList = reg;
		lock.up();
		return false;
	}
	memclear((void*)addr,PAGE_SIZE);
	totalPages++;
	reg->size += PAGE_SIZE;
	objCount = reg->size / objSize;
	lock.up();
	return true;
}

DynArray::~DynArray() {
	if(regions) {
		PageDir::unmapFromCur(regions->addr,regions->size / PAGE_SIZE,true);
		totalPages -= regions->size / PAGE_SIZE;
		/* put region on freelist */
		regions->next = freeList;
		freeList = regions;
		regions = NULL;
	}
}
