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
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/mem/physmem.h>

uintptr_t KHeap::allocAreas(void) {
	/* heap full? */
	if((pages + 1) * PAGE_SIZE > KERNEL_HEAP_SIZE)
		return 0;

	/* allocate one page for area-structs */
	if(paging_map(KERNEL_HEAP_START + pages * PAGE_SIZE,NULL,1,
			PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR | PG_GLOBAL) < 0)
		return 0;

	return KERNEL_HEAP_START + pages++ * PAGE_SIZE;
}

uintptr_t KHeap::allocSpace(size_t count) {
	/* heap full? */
	if((pages + count) * PAGE_SIZE > KERNEL_HEAP_SIZE)
		return 0;

	if(paging_map(KERNEL_HEAP_START + pages * PAGE_SIZE,NULL,count,
			PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR | PG_GLOBAL) < 0)
		return 0;

	pages += count;
	return KERNEL_HEAP_START + (pages - count) * PAGE_SIZE;
}
