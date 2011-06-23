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
#include <sys/mem/kheap.h>
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/video.h>
#include <sys/util.h>
#include <assert.h>
#include <string.h>

#if DEBUGGING
#define DEBUG_ALLOC_N_FREE		0
#endif

/* the number of entries in the occupied map */
#define OCC_MAP_SIZE			1024

#define GUARD_MAGIC				0xDEADBEEF
#define ALIGN(count,align)		(((count) + (align) - 1) & ~((align) - 1))

/* an area in memory */
typedef struct sMemArea sMemArea;
struct sMemArea {
	size_t size;
	void *address;
	sMemArea *next;
};

/**
 * Allocates a new page for areas
 *
 * @return true on success
 */
static bool kheap_loadNewAreas(void);

/**
 * Allocates new space for alloation
 *
 * @param size the minimum size
 * @return true on success
 */
static bool kheap_loadNewSpace(size_t size);

/**
 * Calculates the hash for the given address that should be used as key in occupiedMap
 *
 * @param addr the address
 * @return the key
 */
static size_t kheap_getHash(void *addr);

/* a linked list of free and usable areas. That means the areas have an address and size */
static sMemArea *usableList = NULL;
/* a linked list of free but not usable areas. That means the areas have no address and size */
static sMemArea *freeList = NULL;
/* a hashmap with occupied-lists, key is getHash(address) */
static sMemArea *occupiedMap[OCC_MAP_SIZE] = {NULL};
/* currently occupied memory */
static size_t memUsage = 0;
static size_t pages = 0;

#if DEBUGGING
static bool aafEnabled = false;
#endif

void *kheap_alloc(size_t size) {
	ulong *begin;
	sMemArea *area,*prev,*narea;
	sMemArea **list;

	if(size == 0)
		return NULL;

	/* align and we need 3 ulongs for the guards */
	size = ALIGN(size,sizeof(ulong)) + sizeof(ulong) * 3;

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
		if(!kheap_loadNewSpace(size))
			return NULL;
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
			if(!kheap_loadNewAreas()) {
				/* TODO we may have changed something... */
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
	if(aafEnabled) {
		sFuncCall *trace = util_getKernelStackTrace();
		size_t i = 0;
		vid_printf("[A] %Px %zd ",area->address,area->size);
		while(trace->addr != 0 && i++ < 10) {
			vid_printf("%Px",trace->addr);
			trace++;
			if(trace->addr)
				vid_printf(" ");
		}
		vid_printf("\n");
	}
#endif

	/* insert in occupied-map */
	list = occupiedMap + kheap_getHash(area->address);
	area->next = *list;
	*list = area;

	/* add guards */
	begin = (ulong*)area->address;
	begin[0] = size - sizeof(ulong) * 3;
	begin[1] = GUARD_MAGIC;
	begin[size / sizeof(ulong) - 1] = GUARD_MAGIC;
	return begin + 2;
}

void *kheap_calloc(size_t num,size_t size) {
	void *a = kheap_alloc(num * size);
	if(a == NULL)
		return NULL;

	memclear(a,num * size);
	return a;
}

void kheap_free(void *addr) {
	ulong *begin;
	sMemArea *area,*a,*prev,*next,*oprev,*nprev,*pprev,*tprev;

	/* addr may be null */
	if(addr == NULL)
		return;

	/* check guards */
	begin = (ulong*)addr - 2;
	assert(begin[1] == GUARD_MAGIC);
	assert(begin[begin[0] / sizeof(ulong) + 2] == GUARD_MAGIC);

	/* find the area with given address */
	oprev = NULL;
	area = occupiedMap[kheap_getHash(begin)];
	while(area != NULL) {
		if(area->address == begin)
			break;
		oprev = area;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL)
		return;

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
		occupiedMap[kheap_getHash(begin)] = area->next;

#if DEBUG_ALLOC_N_FREE
	if(aafEnabled) {
		sFuncCall *trace = util_getKernelStackTrace();
		size_t i = 0;
		vid_printf("[F] %Px %zd ",addr,area->size);
		while(trace->addr != 0 && i++ < 10) {
			vid_printf("%Px",trace->addr);
			trace++;
			if(trace->addr)
				vid_printf(" ");
		}
		vid_printf("\n");
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
}

void *kheap_realloc(void *addr,size_t size) {
	ulong *begin;
	sMemArea *area,*a,*prev;

	if(addr == NULL)
		return kheap_alloc(size);

	begin = (ulong*)addr - 2;

	/* find the area with given address */
	area = occupiedMap[kheap_getHash(begin)];
	while(area != NULL) {
		if(area->address == begin)
			break;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL)
		return NULL;

	/* align and we need 3 ulongs for the guards */
	size = ALIGN(size,sizeof(ulong)) + sizeof(ulong) * 3;

	/* ignore shrinks */
	if(size < area->size)
		return addr;

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
				return begin + 2;
			}

			/* makes no sense to continue since we've found the area behind */
			break;
		}
		prev = a;
		a = a->next;
	}

	/* the areas are not big enough, so allocate a new one */
	a = (sMemArea*)kheap_alloc(size);
	if(a == NULL)
		return NULL;

	/* copy the old data and free it */
	memcpy(a,addr,area->size - sizeof(ulong) * 3);
	kheap_free(addr);
	return a;
}

bool kheap_addMemory(uintptr_t addr,size_t size) {
	/* no free areas? */
	if(freeList == NULL) {
		if(!kheap_loadNewAreas())
			return false;
	}

	/* take one area from the freelist and put the memory in it */
	sMemArea *area = freeList;
	freeList = freeList->next;
	area->address = (void*)addr;
	area->size = size;
	/* put area in the usable-list */
	area->next = usableList;
	usableList = area;
	memUsage += size;
	return true;
}

size_t kheap_getPageCount(void) {
	return pages;
}

size_t kheap_getUsedMem(void) {
	size_t i,c = 0;
	sMemArea *a;
	for(i = 0; i < OCC_MAP_SIZE; i++) {
		a = occupiedMap[i];
		while(a != NULL) {
			c += a->size;
			a = a->next;
		}
	}
	return c;
}

size_t kheap_getOccupiedMem(void) {
	return memUsage;
}

size_t kheap_getFreeMem(void) {
	size_t c = 0;
	sMemArea *a = usableList;
	while(a != NULL) {
		c += a->size;
		a = a->next;
	}
	return c;
}

size_t kheap_getAreaSize(void *addr) {
	sMemArea *area;
	area = occupiedMap[kheap_getHash(addr)];
	while(area != NULL) {
		if(area->address == addr)
			return area->size;
		area = area->next;
	}
	return 0;
}

static bool kheap_loadNewSpace(size_t size) {
	uintptr_t addr;
	size_t count;

	/* check for overflow */
	if(size + PAGE_SIZE < PAGE_SIZE)
		return false;

	/* note that we assume here that we won't check the same pages than loadNewAreas() did... */

	/* allocate the required pages */
	count = BYTES_2_PAGES(size);
	addr = kheap_allocSpace(count);
	if(addr == 0)
		return false;

	pages += count;
	return kheap_addMemory(addr,count * PAGE_SIZE);
}

static bool kheap_loadNewAreas(void) {
	sMemArea *area,*end;
	uintptr_t addr;

	addr = kheap_allocAreas();
	if(addr == 0)
		return false;

	/* determine start- and end-address */
	area = (sMemArea*)addr;
	end = area + (PAGE_SIZE / sizeof(sMemArea));

	/* put all areas in the freelist */
	area->next = freeList;
	freeList = area;
	area++;
	while(area < end) {
		area->next = freeList;
		freeList = area;
		area++;
	}

	pages++;
	memUsage += PAGE_SIZE;
	return true;
}

static size_t kheap_getHash(void *addr) {
	/* the algorithm distributes the entries more equally in the occupied-map. */
	/* borrowed from java.util.HashMap :) */
	size_t h = (uintptr_t)addr;
	h ^= (h >> 20) ^ (h >> 12);
	/* note that we can use & (a-1) since OCC_MAP_SIZE = 2^x */
	return (h ^ (h >> 7) ^ (h >> 4)) & (OCC_MAP_SIZE - 1);
}

void kheap_dbg_print(void) {
	sMemArea *area;
	size_t i;

	vid_printf("Used=%zu, free=%zu, pages=%zu\n",kheap_getUsedMem(),kheap_getFreeMem(),
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

#if DEBUGGING

void kheap_dbg_setAaFEnabled(bool enabled) {
	aafEnabled = enabled;
}

#endif
