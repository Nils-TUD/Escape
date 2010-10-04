/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

void dyna_init(sDynArray *d,size_t objSize,uintptr_t areaBegin,size_t areaSize) {
	d->objSize = objSize;
	d->areaBegin = areaBegin;
	d->areaSize = areaSize;
	d->objCount = 0;
	d->pageCount = 0;
}

bool dyna_extend(sDynArray *d) {
	uintptr_t addr = d->areaBegin + d->pageCount * PAGE_SIZE;
	if(mm_getFreeFrames(MM_DEF) == 0 || d->pageCount * PAGE_SIZE >= d->areaSize)
		return false;
	paging_map(addr,NULL,1,PG_SUPERVISOR | PG_WRITABLE | PG_PRESENT);
	memset((void*)addr,0,PAGE_SIZE);
	d->pageCount++;
	d->objCount = (d->pageCount * PAGE_SIZE) / d->objSize;
	return true;
}

void dyna_destroy(sDynArray *d) {
	paging_unmap(d->areaBegin,d->pageCount,true);
}
