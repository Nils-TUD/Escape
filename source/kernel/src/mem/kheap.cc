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
#include <sys/mem/pagedir.h>
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
	if(size == 0)
		return NULL;

	/* align and we need 3 ulongs for the guards */
	size = ROUND_UP(size,sizeof(ulong)) + sizeof(ulong) * 3;

	SpinLock::acquire(&lock);

	/* find a suitable area */
	MemArea *prev = NULL;
	MemArea *area = usableList;
	while(area != NULL) {
		if(area->size >= size)
			break;
		prev = area;
		area = area->next;
	}

	/* no area found? */
	if(area == NULL) {
		if(!loadNewSpace(size)) {
			SpinLock::release(&lock);
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
				SpinLock::release(&lock);
				return NULL;
			}
		}

		/* split the area */
		MemArea *narea = freeList;
		freeList = freeList->next;
		narea->address = (void*)((uintptr_t)area->address + size);
		narea->size = area->size - size;
		area->size = size;
		/* insert in usable-list */
		narea->next = usableList;
		usableList = narea;
	}

	/* insert in occupied-map */
	MemArea **list = occupiedMap + getHash(area->address);
	area->next = *list;
	*list = area;

	/* add guards */
	ulong *begin = (ulong*)area->address;
	begin[0] = size - sizeof(ulong) * 3;
	begin[1] = GUARD_MAGIC;
	begin[size / sizeof(ulong) - 1] = GUARD_MAGIC;
	SpinLock::release(&lock);
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
	/* addr may be null */
	if(addr == NULL)
		return;

	/* check guards */
	ulong *begin = (ulong*)addr - 2;
	assert(begin[1] == GUARD_MAGIC);
	assert(begin[begin[0] / sizeof(ulong) + 2] == GUARD_MAGIC);

	SpinLock::acquire(&lock);

	/* find the area with given address */
	MemArea *oprev = NULL;
	MemArea *area = occupiedMap[getHash(begin)];
	while(area != NULL) {
		if(area->address == begin)
			break;
		oprev = area;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL) {
		SpinLock::release(&lock);
		return;
	}

	/* find the previous and next free areas */
	MemArea *prev = NULL;
	MemArea *next = NULL;
	MemArea *tprev = NULL;
	MemArea *pprev = NULL;
	MemArea *nprev = NULL;
	MemArea *a = usableList;
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
	SpinLock::release(&lock);
}

void *KHeap::realloc(void *addr,size_t size) {
	if(addr == NULL)
		return alloc(size);

	ulong *begin = (ulong*)addr - 2;

	SpinLock::acquire(&lock);

	/* find the area with given address */
	MemArea *area = occupiedMap[getHash(begin)];
	while(area != NULL) {
		if(area->address == begin)
			break;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL) {
		SpinLock::release(&lock);
		return NULL;
	}

	/* align and we need 3 ulongs for the guards */
	size = ROUND_UP(size,sizeof(ulong)) + sizeof(ulong) * 3;

	/* ignore shrinks */
	if(size < area->size) {
		SpinLock::release(&lock);
		return addr;
	}

	MemArea *a = usableList;
	MemArea *prev = NULL;
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
				SpinLock::release(&lock);
				return begin + 2;
			}

			/* makes no sense to continue since we've found the area behind */
			break;
		}
		prev = a;
		a = a->next;
	}
	SpinLock::release(&lock);

	/* the areas are not big enough, so allocate a new one */
	a = (MemArea*)alloc(size);
	if(a == NULL)
		return NULL;

	/* copy the old data and free it */
	memcpy(a,addr,area->size - sizeof(ulong) * 3);
	free(addr);
	return a;
}

size_t KHeap::getUsedMem() {
	size_t c = 0;
	for(size_t i = 0; i < OCC_MAP_SIZE; i++) {
		MemArea *a = occupiedMap[i];
		while(a != NULL) {
			c += a->size;
			a = a->next;
		}
	}
	return c;
}

size_t KHeap::getFreeMem() {
	size_t c = 0;
	MemArea *a = usableList;
	while(a != NULL) {
		c += a->size;
		a = a->next;
	}
	return c;
}

void KHeap::print(OStream &os) {
	os.writef("Used=%zu, free=%zu, pages=%zu\n",getUsedMem(),getFreeMem(),
			memUsage / PAGE_SIZE);
	os.writef("UsableList:\n");
	MemArea *area = usableList;
	while(area != NULL) {
		os.writef("\t%p: addr=%p, size=0x%zx\n",area,area->address,area->size);
		area = area->next;
	}

	os.writef("OccupiedMap:\n");
	for(size_t i = 0; i < OCC_MAP_SIZE; i++) {
		area = occupiedMap[i];
		if(area != NULL) {
			os.writef("\t%d:\n",i);
			while(area != NULL) {
				os.writef("\t\t%p: addr=%p, size=0x%zx\n",area,area->address,area->size);
				area = area->next;
			}
		}
	}
}

bool KHeap::doAddMemory(uintptr_t addr,size_t size) {
	if(freeList == NULL) {
		if(!loadNewAreas())
			return false;
	}

	/* take one area from the freelist and put the memory in it */
	MemArea *area = freeList;
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
	/* check for overflow */
	if(size + PAGE_SIZE < PAGE_SIZE)
		return false;

	/* note that we assume here that we won't check the same pages than loadNewAreas() did... */

	/* allocate the required pages */
	size_t count = BYTES_2_PAGES(size);
	uintptr_t addr = allocSpace(count);
	if(addr == 0)
		return false;

	return doAddMemory(addr,count * PAGE_SIZE);
}

bool KHeap::loadNewAreas() {
	uintptr_t addr = allocAreas();
	if(addr == 0)
		return false;

	/* determine start- and end-address */
	MemArea *area = (MemArea*)addr;
	MemArea *end = area + (PAGE_SIZE / sizeof(MemArea));

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
