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

/* the number of entries in the occupied map */
#define OCC_MAP_SIZE			1024
#ifdef __eco32__
#define DEBUG_ALLOC_N_FREE		0
/* TODO we need the alignment */
#define DEBUG_ADD_GUARDS		1
#elif DEBUGGING
#define DEBUG_ALLOC_N_FREE		0
#define DEBUG_ADD_GUARDS		1
#endif

#define GUARD_MAGIC				0xDEADBEEF
#define ALIGN(count,align)		(((count) + (align) - 1) & ~((align) - 1))

#if DEBUG_ADD_GUARDS
void *_kheap_alloc(size_t size);
void *_kheap_calloc(size_t num,size_t size);
void * _kheap_realloc(void *addr,size_t size);
void _kheap_free(void *addr);
#	define kheap_alloc_guard(size)			kheap_alloc(size)
#	define kheap_calloc_guard(num,size)		kheap_calloc(num,size)
#	define kheap_realloc_guard(addr,size)	kheap_realloc(addr,size)
#	define kheap_free_guard(addr)			kheap_free(addr)
#else
#	define _kheap_alloc(size)				kheap_alloc(size)
#	define _kheap_calloc(num,size)			kheap_calloc(num,size)
#	define _kheap_realloc(addr,size)		kheap_realloc(addr,size)
#	define _kheap_free(addr)				kheap_free(addr)
#endif

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
/* number of currently occupied pages */
static size_t pages = 0;

#if DEBUGGING
static bool aafEnabled = false;
#endif

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
	return pages * PAGE_SIZE;
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

#if DEBUG_ADD_GUARDS
void *kheap_alloc_guard(size_t size) {
	ulong *a;
	size = ALIGN(size,sizeof(ulong));
	a = (ulong*)_kheap_alloc(size + sizeof(ulong) * 3);
	if(a) {
		a[0] = GUARD_MAGIC;
		a[1] = size;
		a[size / sizeof(ulong) + 2] = GUARD_MAGIC;
		return a + 2;
	}
	return NULL;
}

void *kheap_calloc_guard(size_t num,size_t size) {
	ulong *a;
	size = ALIGN(num * size,sizeof(ulong));
	a = (ulong*)_kheap_alloc(size + sizeof(ulong) * 3);
	if(a) {
		a[0] = GUARD_MAGIC;
		a[1] = size;
		a[size / sizeof(ulong) + 2] = GUARD_MAGIC;
		memclear(a + 2,size);
		return a + 2;
	}
	return NULL;
}

void *kheap_realloc_guard(void *addr,size_t size) {
	ulong *a = (ulong*)addr;
	size = ALIGN(size,sizeof(ulong));
	if(a) {
		assert(a[-2] == GUARD_MAGIC);
		assert(a[a[-1] / sizeof(ulong)] == GUARD_MAGIC);
		a = _kheap_realloc(a - 2,size + sizeof(ulong) * 3);
	}
	else
		a = _kheap_realloc(NULL,size + sizeof(ulong) * 3);
	if(a) {
		a[0] = GUARD_MAGIC;
		a[1] = size;
		a[size / sizeof(ulong) + 2] = GUARD_MAGIC;
		return a + 2;
	}
	return NULL;
}

void kheap_free_guard(void *addr) {
	ulong *a = (ulong*)addr;
	if(a) {
		assert(a[-2] == GUARD_MAGIC);
		assert(a[a[-1] / sizeof(ulong)] == GUARD_MAGIC);
		_kheap_free(a - 2);
	}
}
#endif

void *_kheap_alloc(size_t size) {
	sMemArea *area,*prev,*narea;
	sMemArea **list;

	if(size == 0)
		return NULL;

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
		vid_printf("[A] %Px %Sd ",area->address,area->size);
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

	return area->address;
}

void *_kheap_calloc(size_t num,size_t size) {
	void *a = _kheap_alloc(num * size);
	if(a == NULL)
		return NULL;

	memclear(a,num * size);
	return a;
}

void _kheap_free(void *addr) {
	sMemArea *area,*a,*prev,*next,*oprev,*nprev,*pprev,*tprev;

	/* addr may be null */
	if(addr == NULL)
		return;

	/* find the area with given address */
	oprev = NULL;
	area = occupiedMap[kheap_getHash(addr)];
	while(area != NULL) {
		if(area->address == addr)
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
		if((uintptr_t)a->address + a->size == (uintptr_t)addr) {
			prev = a;
			pprev = tprev;
		}
		if((uintptr_t)a->address == (uintptr_t)addr + area->size) {
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
		occupiedMap[kheap_getHash(addr)] = area->next;

#if DEBUG_ALLOC_N_FREE
	if(aafEnabled) {
		sFuncCall *trace = util_getKernelStackTrace();
		size_t i = 0;
		vid_printf("[F] %Px %Sd ",addr,area->size);
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

void *_kheap_realloc(void *addr,size_t size) {
	sMemArea *area,*a,*prev;

	if(addr == NULL)
		return _kheap_alloc(size);

	/* find the area with given address */
	area = occupiedMap[kheap_getHash(addr)];
	while(area != NULL) {
		if(area->address == addr)
			break;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL)
		return NULL;

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
				return addr;
			}

			/* makes no sense to continue since we've found the area behind */
			break;
		}
		prev = a;
		a = a->next;
	}

	/* the areas are not big enough, so allocate a new one */
	a = (sMemArea*)_kheap_alloc(size);
	if(a == NULL)
		return NULL;

	/* copy the old data and free it */
	memcpy(a,addr,area->size);
	_kheap_free(addr);
	return a;
}

static bool kheap_loadNewSpace(size_t size) {
	sMemArea *area;
	uintptr_t addr;
	size_t count;

	/* no free areas? */
	if(freeList == NULL) {
		if(!kheap_loadNewAreas())
			return false;
	}

	/* check for overflow */
	if(size + PAGE_SIZE < PAGE_SIZE)
		return false;

	/* note that we assume here that we won't check the same pages than loadNewAreas() did... */

	/* allocate the required pages */
	count = BYTES_2_PAGES(size);
	addr = kheap_allocSpace(count);
	if(addr == 0)
		return false;

	/* take one area from the freelist and put the memory in it */
	area = freeList;
	freeList = freeList->next;
	area->address = (void*)addr;
	area->size = PAGE_SIZE * count;
	/* put area in the usable-list */
	area->next = usableList;
	usableList = area;

	pages += count;
	return true;
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


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void kheap_dbg_setAaFEnabled(bool enabled) {
	aafEnabled = enabled;
}

void kheap_dbg_print(void) {
	sMemArea *area;
	size_t i;

	vid_printf("Used=%Su, free=%Su, pages=%Su\n",kheap_getUsedMem(),kheap_getFreeMem(),pages);
	vid_printf("UsableList:\n");
	area = usableList;
	while(area != NULL) {
		vid_printf("\t%p: addr=%p, size=0x%Sx\n",area,area->address,area->size);
		area = area->next;
	}

	vid_printf("OccupiedMap:\n");
	for(i = 0; i < OCC_MAP_SIZE; i++) {
		area = occupiedMap[i];
		if(area != NULL) {
			vid_printf("\t%d:\n",i);
			while(area != NULL) {
				vid_printf("\t\t%p: addr=%p, size=0x%Sx\n",area,area->address,area->size);
				area = area->next;
			}
		}
	}
}

#endif
