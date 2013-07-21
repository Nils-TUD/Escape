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
#include <sys/mem/physmem.h>
#include <sys/mem/paging.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <sys/util.h>
#include <assert.h>
#include <string.h>

KHeap::MemArea *KHeap::usableList = NULL;
KHeap::MemArea *KHeap::freeList = NULL;
KHeap::MemArea *KHeap::occupiedMap[OCC_MAP_SIZE] = {NULL};
size_t KHeap::memUsage = 0;
size_t KHeap::pages = 0;
klock_t KHeap::lock;

void *KHeap::alloc(size_t size) {
	ulong *begin;
	MemArea *area,*prev,*narea;
	MemArea **list;

	if(size == 0)
		return NULL;

	/* align and we need 3 ulongs for the guards */
	size = ROUND_UP(size,sizeof(ulong)) + sizeof(ulong) * 3;

	spinlock_aquire(&lock);

	/* find a suitable area */
	prev = NULL;
	area = usableList;
	while(area != NULL) {
		if(area->size >= size)
			break;
		prev = area;
		area = area->next;
	}

	/* no area found? */
	if(area == NULL) {
		if(!loadNewSpace(size)) {
			spinlock_release(&lock);
			return NULL;
		}
		/* we can assume that it fits */
		area = usableList;
		/* remove from usable-list */
		usableList = area->next;
	}
	else {
		/* remove from usable-list */
		if(prev == NULL)
			usableList = area->next;
		else
			prev->next = area->next;
	}

	/* is there space left? */
	if(size < area->size) {
		if(freeList == NULL) {
			if(!loadNewAreas()) {
				/* TODO we may have changed something... */
				spinlock_release(&lock);
				return NULL;
			}
		}

		/* split the area */
		narea = freeList;
		freeList = freeList->next;
		narea->address = (void*)((uintptr_t)area->address + size);
		narea->size = area->size - size;
		area->size = size;
		/* insert in usable-list */
		narea->next = usableList;
		usableList = narea;
	}

	/* insert in occupied-map */
	list = occupiedMap + getHash(area->address);
	area->next = *list;
	*list = area;

	/* add guards */
	begin = (ulong*)area->address;
	begin[0] = size - sizeof(ulong) * 3;
	begin[1] = GUARD_MAGIC;
	begin[size / sizeof(ulong) - 1] = GUARD_MAGIC;
	spinlock_release(&lock);
	return begin + 2;
}

void *KHeap::calloc(size_t num,size_t size) {
	void *a = alloc(num * size);
	if(a == NULL)
		return NULL;

	memclear(a,num * size);
	return a;
}

void KHeap::free(void *addr) {
	ulong *begin;
	MemArea *area,*a,*prev,*next,*oprev,*nprev,*pprev,*tprev;

	/* addr may be null */
	if(addr == NULL)
		return;

	/* check guards */
	begin = (ulong*)addr - 2;
	assert(begin[1] == GUARD_MAGIC);
	assert(begin[begin[0] / sizeof(ulong) + 2] == GUARD_MAGIC);

	spinlock_aquire(&lock);

	/* find the area with given address */
	oprev = NULL;
	area = occupiedMap[getHash(begin)];
	while(area != NULL) {
		if(area->address == begin)
			break;
		oprev = area;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL) {
		spinlock_release(&lock);
		return;
	}

	/* find the previous and next free areas */
	prev = NULL;
	next = NULL;
	tprev = NULL;
	pprev = NULL;
	nprev = NULL;
	a = usableList;
	while(a != NULL) {
		if((uintptr_t)a->address + a->size == (uintptr_t)begin) {
			prev = a;
			pprev = tprev;
		}
		if((uintptr_t)a->address == (uintptr_t)begin + area->size) {
			next = a;
			nprev = tprev;
		}
		/* do we have both? */
		if(prev && next)
			break;
		tprev = a;
		a = a->next;
	}

	/* remove area from occupied-map */
	if(oprev)
		oprev->next = area->next;
	else
		occupiedMap[getHash(begin)] = area->next;

	/* see what we have to merge */
	if(prev && next) {
		/* merge prev & area & next */
		area->size += prev->size + next->size;
		area->address = prev->address;
		/* remove prev and next from the list */
		if(nprev)
			nprev->next = next->next;
		else
			usableList = next->next;
		/* special-case if next is the previous of prev */
		if(pprev == next) {
			if(nprev)
				nprev->next = prev->next;
			else
				usableList = prev->next;
		}
		else {
			if(pprev)
				pprev->next = prev->next;
			else
				usableList = prev->next;
		}
		/* put area on the usable-list */
		area->next = usableList;
		usableList = area;
		/* put prev and next on the freelist */
		prev->next = next;
		next->next = freeList;
		freeList = prev;
	}
	else if(prev) {
		/* merge preg & area */
		prev->size += area->size;
		/* put area on the freelist */
		area->next = freeList;
		freeList = area;
	}
	else if(next) {
		/* merge area & next */
		next->address = area->address;
		next->size += area->size;
		/* put area on the freelist */
		area->next = freeList;
		freeList = area;
	}
	else {
		/* insert area in usableList */
		area->next = usableList;
		usableList = area;
	}
	spinlock_release(&lock);
}

void *KHeap::realloc(void *addr,size_t size) {
	ulong *begin;
	MemArea *area,*a,*prev;

	if(addr == NULL)
		return alloc(size);

	begin = (ulong*)addr - 2;

	spinlock_aquire(&lock);

	/* find the area with given address */
	area = occupiedMap[getHash(begin)];
	while(area != NULL) {
		if(area->address == begin)
			break;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL) {
		spinlock_release(&lock);
		return NULL;
	}

	/* align and we need 3 ulongs for the guards */
	size = ROUND_UP(size,sizeof(ulong)) + sizeof(ulong) * 3;

	/* ignore shrinks */
	if(size < area->size) {
		spinlock_release(&lock);
		return addr;
	}

	a = usableList;
	prev = NULL;
	while(a != NULL) {
		/* found the area behind? */
		if((uintptr_t)a->address == (uintptr_t)area->address + area->size) {
			/* if the size of both is big enough we can use them */
			if(area->size + a->size >= size) {
				/* space left? */
				if(size < area->size + a->size) {
					/* so move the area forward */
					a->address = (void*)((uintptr_t)area->address + size);
					a->size = (area->size + a->size) - size;
				}
				/* otherwise we don't need a anymore */
				else {
					/* remove a from usable-list */
					if(prev == NULL)
						usableList = a->next;
					else
						prev->next = a->next;
					/* free a */
					a->next = freeList;
					freeList = a;
				}

				area->size = size;
				/* reset guards */
				begin[0] = size - sizeof(ulong) * 3;
				begin[1] = GUARD_MAGIC;
				begin[size / sizeof(ulong) - 1] = GUARD_MAGIC;
				spinlock_release(&lock);
				return begin + 2;
			}

			/* makes no sense to continue since we've found the area behind */
			break;
		}
		prev = a;
		a = a->next;
	}
	spinlock_release(&lock);

	/* the areas are not big enough, so allocate a new one */
	a = (MemArea*)alloc(size);
	if(a == NULL)
		return NULL;

	/* copy the old data and free it */
	memcpy(a,addr,area->size - sizeof(ulong) * 3);
	free(addr);
	return a;
}

size_t KHeap::getUsedMem(void) {
	size_t i,c = 0;
	MemArea *a;
	for(i = 0; i < OCC_MAP_SIZE; i++) {
		a = occupiedMap[i];
		while(a != NULL) {
			c += a->size;
			a = a->next;
		}
	}
	return c;
}

size_t KHeap::getFreeMem(void) {
	size_t c = 0;
	MemArea *a = usableList;
	while(a != NULL) {
		c += a->size;
		a = a->next;
	}
	return c;
}

void KHeap::print(void) {
	MemArea *area;
	size_t i;

	vid_printf("Used=%zu, free=%zu, pages=%zu\n",getUsedMem(),getFreeMem(),
			memUsage / PAGE_SIZE);
	vid_printf("UsableList:\n");
	area = usableList;
	while(area != NULL) {
		vid_printf("\t%p: addr=%p, size=0x%zx\n",area,area->address,area->size);
		area = area->next;
	}

	vid_printf("OccupiedMap:\n");
	for(i = 0; i < OCC_MAP_SIZE; i++) {
		area = occupiedMap[i];
		if(area != NULL) {
			vid_printf("\t%d:\n",i);
			while(area != NULL) {
				vid_printf("\t\t%p: addr=%p, size=0x%zx\n",area,area->address,area->size);
				area = area->next;
			}
		}
	}
}

bool KHeap::doAddMemory(uintptr_t addr,size_t size) {
	MemArea *area;
	if(freeList == NULL) {
		if(!loadNewAreas())
			return false;
	}

	/* take one area from the freelist and put the memory in it */
	area = freeList;
	freeList = freeList->next;
	area->address = (void*)addr;
	area->size = size;
	/* put area in the usable-list */
	area->next = usableList;
	usableList = area;
	memUsage += size;
	return true;
}

bool KHeap::loadNewSpace(size_t size) {
	uintptr_t addr;
	size_t count;

	/* check for overflow */
	if(size + PAGE_SIZE < PAGE_SIZE)
		return false;

	/* note that we assume here that we won't check the same pages than loadNewAreas() did... */

	/* allocate the required pages */
	count = BYTES_2_PAGES(size);
	addr = allocSpace(count);
	if(addr == 0)
		return false;

	return doAddMemory(addr,count * PAGE_SIZE);
}

bool KHeap::loadNewAreas(void) {
	MemArea *area,*end;
	uintptr_t addr;

	addr = allocAreas();
	if(addr == 0)
		return false;

	/* determine start- and end-address */
	area = (MemArea*)addr;
	end = area + (PAGE_SIZE / sizeof(MemArea));

	/* put all areas in the freelist */
	area->next = freeList;
	freeList = area;
	area++;
	while(area < end) {
		area->next = freeList;
		freeList = area;
		area++;
	}

	memUsage += PAGE_SIZE;
	return true;
}

size_t KHeap::getHash(void *addr) {
	/* the algorithm distributes the entries more equally in the occupied-map. */
	/* borrowed from java.util.HashMap :) */
	size_t h = (uintptr_t)addr;
	h ^= (h >> 20) ^ (h >> 12);
	/* note that we can use & (a-1) since OCC_MAP_SIZE = 2^x */
	return (h ^ (h >> 7) ^ (h >> 4)) & (OCC_MAP_SIZE - 1);
}
