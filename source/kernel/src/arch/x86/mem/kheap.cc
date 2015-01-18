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

#include <mem/kheap.h>
#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <common.h>

uintptr_t KHeap::allocAreas() {
	/* heap full? */
	if((pages + 1) * PAGE_SIZE > KHEAP_SIZE)
		return 0;

	/* allocate one page for area-structs */
	PageTables::KAllocator alloc;
	if(PageDir::mapToCur(KHEAP_START + pages * PAGE_SIZE,1,alloc,
			PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR | PG_GLOBAL) < 0)
		return 0;

	return KHEAP_START + pages++ * PAGE_SIZE;
}

uintptr_t KHeap::allocSpace(size_t count) {
	/* heap full? */
	if((pages + count) * PAGE_SIZE > KHEAP_SIZE)
		return 0;

	PageTables::KAllocator alloc;
	if(PageDir::mapToCur(KHEAP_START + pages * PAGE_SIZE,count,alloc,
			PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR | PG_GLOBAL) < 0)
		return 0;

	pages += count;
	return KHEAP_START + (pages - count) * PAGE_SIZE;
}
