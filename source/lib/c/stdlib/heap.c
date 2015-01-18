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

#include <sys/arch.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/mman.h>
#include <sys/sync.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if DEBUGGING
#define DEBUG_ALLOC_N_FREE		0
#define DEBUG_ALLOC_N_FREE_PID	27	/* -1 = all */
#endif

#define GUARD_MAGIC				0xDEADBEEF
#define FREE_MAGIC				0xFEEEFEEE

/* an area in memory */
typedef struct sMemArea sMemArea;
struct sMemArea {
	size_t size;
	void *address;
	sMemArea *next;
};

void initHeap(void);

/**
 * Allocates a new page for areas
 *
 * @return true on success
 */
static bool loadNewAreas(void);

/**
 * Allocates new space for alloation
 *
 * @param size the minimum size
 * @return true on success
 */
static bool loadNewSpace(size_t size);

/* a linked list of free and usable areas. That means the areas have an address and size */
static sMemArea *usableList = NULL;
/* a linked list of free but not usable areas. That means the areas have no address and size */
static sMemArea *freeList = NULL;
/* total number of pages we're using */
static uintptr_t heapstart = 0;
static size_t pageCount = 0;

/* the lock for the heap */
static tUserSem heapSem;
static bool initialized = false;

void initHeap(void) {
	if(initialized)
		return;

	if(usemcrt(&heapSem,1) < 0)
		error("Unable to create heap lock");
	initialized = true;
}

void *malloc(size_t size) {
	ulong *begin;
	sMemArea *area,*prev,*narea;

	if(size == 0)
		return NULL;

	usemdown(&heapSem);

	/* align and we need 3 ulongs for the guards */
	size = ROUND_UP(size,sizeof(ulong)) + sizeof(ulong) * 4;

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
			usemup(&heapSem);
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
				usemup(&heapSem);
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

#if DEBUG_ALLOC_N_FREE
	if(DEBUG_ALLOC_N_FREE_PID == -1 || getpid() == DEBUG_ALLOC_N_FREE_PID) {
		size_t i = 0;
		uintptr_t *trace = getStackTrace();
		debugf("[A] %x %d ",(char*)area->address + 3,area->size);
		while(*trace && i++ < 10) {
			debugf("%x",*trace);
			if(trace[1])
				debugf(" ");
			trace++;
		}
		debugf("\n");
	}
#endif

	/* add guards */
	begin = (ulong*)area->address;
	begin[0] = (ulong)area;
	begin[1] = size - sizeof(ulong) * 4;
	begin[2] = GUARD_MAGIC;
	begin[size / sizeof(ulong) - 1] = GUARD_MAGIC;
	usemup(&heapSem);
	return begin + 3;
}

void *calloc(size_t num,size_t size) {
	void *a = malloc(num * size);
	if(a == NULL)
		return NULL;

	memclear(a,num * size);
	return a;
}

void free(void *addr) {
	ulong *begin;
	sMemArea *area,*a,*prev,*next,*nprev,*pprev,*tprev;

	/* addr may be null */
	if(addr == NULL)
		return;

	/* check guards */
	begin = (ulong*)addr - 3;
	area = (sMemArea*)begin[0];
	vassert(begin[0] != FREE_MAGIC,"Duplicate free of %p?",addr);
	assert(begin[2] == GUARD_MAGIC);
	assert(begin[begin[1] / sizeof(ulong) + 3] == GUARD_MAGIC);

	usemdown(&heapSem);

	/* mark as free */
	begin[0] = FREE_MAGIC;

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

#if DEBUG_ALLOC_N_FREE
	if(DEBUG_ALLOC_N_FREE_PID == -1 || getpid() == DEBUG_ALLOC_N_FREE_PID) {
		size_t i = 0;
		uintptr_t *trace = getStackTrace();
		debugf("[F] %x %d ",addr,area->size);
		while(*trace && i++ < 10) {
			debugf("%x",*trace);
			if(trace[1])
				debugf(" ");
			trace++;
		}
		debugf("\n");
	}
#endif

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

	usemup(&heapSem);
}

void *realloc(void *addr,size_t size) {
	ulong *begin;
	sMemArea *area,*a,*prev;
	if(addr == NULL)
		return malloc(size);

	begin = (ulong*)addr - 3;
	area = (sMemArea*)begin[0];
	/* check guards */
	vassert(begin[0] != FREE_MAGIC,"Duplicate free?");
	assert(begin[2] == GUARD_MAGIC);
	assert(begin[begin[1] / sizeof(ulong) + 3] == GUARD_MAGIC);

	usemdown(&heapSem);

	/* align and we need 3 ulongs for the guards */
	size = ROUND_UP(size,sizeof(ulong)) + sizeof(ulong) * 4;

	/* ignore shrinks */
	if(size < area->size) {
		usemup(&heapSem);
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
				begin[0] = (ulong)area;
				begin[1] = size - sizeof(ulong) * 4;
				begin[2] = GUARD_MAGIC;
				begin[size / sizeof(ulong) - 1] = GUARD_MAGIC;
				usemup(&heapSem);
				return begin + 3;
			}

			/* makes no sense to continue since we've found the area behind */
			break;
		}
		prev = a;
		a = a->next;
	}

	usemup(&heapSem);

	/* the areas are not big enough, so allocate a new one */
	a = malloc(size);
	if(a == NULL)
		return NULL;

	/* copy the old data and free it */
	memcpy(a,addr,area->size - sizeof(ulong) * 4);
	free(addr);
	return a;
}

static bool loadNewSpace(size_t size) {
	void *oldEnd;
	size_t count;
	sMemArea *area;

	/* no free areas? */
	if(freeList == NULL) {
		if(!loadNewAreas())
			return false;
	}

	/* check for overflow */
	if(size + PAGE_SIZE < PAGE_SIZE)
		return false;

	/* allocate the required pages */
	count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	oldEnd = chgsize(count);
	if(oldEnd == NULL)
		return false;

	if(pageCount == 0)
		heapstart = (uintptr_t)oldEnd;

	pageCount += count;
	/* take one area from the freelist and put the memory in it */
	area = freeList;
	freeList = freeList->next;
	area->address = oldEnd;
	area->size = PAGE_SIZE * count;
	/* put area in the usable-list */
	area->next = usableList;
	usableList = area;
	return true;
}

static bool loadNewAreas(void) {
	sMemArea *area,*end;
	void *oldEnd;

	/* allocate one page for area-structs */
	oldEnd = chgsize(1);
	if(oldEnd == NULL)
		return false;

	if(pageCount == 0)
		heapstart = (uintptr_t)oldEnd;

	/* determine start- and end-address */
	pageCount++;
	area = (sMemArea*)oldEnd;
	end = area + (PAGE_SIZE / sizeof(sMemArea));

	/* put all areas in the freelist */
	freeList = area;
	area->next = NULL;
	area++;
	while(area < end) {
		area->next = freeList;
		freeList = area;
		area++;
	}

	return true;
}

bool isOnHeap(const void *ptr) {
	return (uintptr_t)ptr >= heapstart && (uintptr_t)ptr < heapstart + pageCount * PAGE_SIZE;
}

size_t heapspace(void) {
	sMemArea *area;
	size_t c = 0;
	area = usableList;
	while(area != NULL) {
		c += area->size;
		area = area->next;
	}
	return c;
}

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void printheap(void) {
	sMemArea *area;

	printf("PageCount=%d\n",pageCount);
	printf("UsableList:\n");
	area = usableList;
	while(area != NULL) {
		printf("\t%p: addr=%p, size=0x%x, next=%p\n",area,area->address,area->size,area->next);
		area = area->next;
	}

	/*printf("FreeList:\n");
	area = freeList;
	while(area != NULL) {
		printf("\t0x%x: addr=0x%x, size=0x%x, next=0x%x\n",area,area->address,area->size,area->next);
		area = area->next;
	}*/
}

#endif
