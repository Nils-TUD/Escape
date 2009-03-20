/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/kheap.h"
#include "../h/paging.h"
#include <string.h>
#include "../h/video.h"

/* the number of entries in the occupied map */
#define OCC_MAP_SIZE			1024

/* an area in memory */
typedef struct sMemArea sMemArea;
struct sMemArea {
	u32 size;
	void *address;
	sMemArea *next;
};

/* a linked list of free and usable areas. That means the areas have an address and size */
static sMemArea *usableList = NULL;
/* a linked list of free but not usable areas. That means the areas have no address and size */
static sMemArea *freeList = NULL;
/* a hashmap with occupied-lists, key is (address % OCC_MAP_SIZE) */
static sMemArea *occupiedMap[OCC_MAP_SIZE] = {NULL};

static u32 pages = 0;

/**
 * Allocates a new area. Assumes that there are any
 *
 * @return the area
 */
static sMemArea *kheap_allocArea(void);

/**
 * Releases the given area
 *
 * @param area the area
 */
static void kheap_freeArea(sMemArea *area);

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
static bool kheap_loadNewSpace(u32 size);

u32 kheap_getFreeMem(void) {
	u32 c = 0;
	sMemArea *a = usableList;
	while(a != NULL) {
		c += a->size;
		a = a->next;
	}
	return c;
}

void kheap_init(void) {

}

u32 kheap_getAreaSize(void *addr) {
	sMemArea *area;
	area = occupiedMap[(u32)addr % OCC_MAP_SIZE];
	while(area != NULL) {
		if(area->address == addr)
			return area->size;
		area = area->next;
	}
	return 0;
}

void *kheap_alloc(u32 size) {
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
		narea = kheap_allocArea();
		if(narea == NULL)
			return NULL;

		narea->address = (void*)((u32)area->address + size);
		narea->size = area->size - size;
		area->size = size;
		/* insert in usable-list */
		narea->next = usableList;
		usableList = narea;
	}

	/* insert in occupied-map */
	list = occupiedMap + ((u32)area->address % OCC_MAP_SIZE);
	area->next = *list;
	*list = area;

	return area->address;
}

void kheap_free(void *addr) {
	sMemArea *area,*oprev;
	sMemArea *a,*prev,*next,*nprev,*pprev,*tprev;

	/* addr may be null */
	if(addr == NULL)
		return;

	/* find the area with given address */
	oprev = NULL;
	area = occupiedMap[(u32)addr % OCC_MAP_SIZE];
	while(area != NULL) {
		if(area->address == addr)
			break;
		oprev = area;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL)
		return;

	/* remove area from occupied-map */
	if(oprev)
		oprev->next = area->next;
	else
		occupiedMap[(u32)addr % OCC_MAP_SIZE] = area->next;

	/* look what we can merge */
	/* find the previous and next free areas */
	prev = NULL;
	next = NULL;
	tprev = NULL;
	a = usableList;
	while(a != NULL) {
		if((u32)a->address + a->size == (u32)addr) {
			prev = a;
			pprev = tprev;
		}
		if((u32)a->address == (u32)addr + area->size) {
			next = a;
			nprev = tprev;
		}
		/* do we have both? */
		if(prev && next)
			break;
		tprev = a;
		a = a->next;
	}

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
		kheap_freeArea(prev);
		kheap_freeArea(next);
	}
	else if(prev) {
		/* merge preg & area */
		prev->size += area->size;
		/* put area on the freelist */
		kheap_freeArea(area);
	}
	else if(next) {
		/* merge area & next */
		next->address = area->address;
		next->size += area->size;
		/* put area on the freelist */
		kheap_freeArea(area);
	}
	else {
		/* insert area in usableList */
		area->next = usableList;
		usableList = area;
	}
}

void *kheap_realloc(void *addr,u32 size) {
	sMemArea *area,*a,*prev;
	/* find the area with given address */
	area = occupiedMap[(u32)addr % OCC_MAP_SIZE];
	while(area != NULL) {
		if(area->address == addr)
			break;
		area = area->next;
	}

	/* area not found? */
	if(area == NULL)
		return NULL;

	a = usableList;
	prev = NULL;
	while(a != NULL) {
		/* found the area behind? */
		if(a->address == (u8*)area->address + area->size) {
			/* if the size of both is big enough we can use them */
			if(area->size + a->size >= size) {
				/* space left? */
				if(size < area->size + a->size) {
					/* so move the area forward */
					a->address = (void*)((u32)area->address + size);
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
					kheap_freeArea(a);
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
	a = kheap_alloc(size);
	if(a == NULL)
		return NULL;

	/* copy the old data and free it */
	memcpy(a,addr,area->size);
	kheap_free(addr);
	return a;
}

static sMemArea *kheap_allocArea(void) {
	sMemArea *area;

	/* no free area anymore? */
	if(freeList == NULL)
		return NULL;

	/* remove from freelist */
	area = freeList;
	freeList = freeList->next;

	return area;
}

static void kheap_freeArea(sMemArea *area) {
	/* insert in freelist */
	area->next = freeList;
	freeList = area;
}

static bool kheap_loadNewSpace(u32 size) {
	sMemArea *area;
	s32 count;

	/* no free areas? */
	if(freeList == NULL) {
		if(!kheap_loadNewAreas())
			return false;
	}

	/* note that we assume here that we won't check the same pages than loadNewAreas() did... */

	/* allocate the required pages */
	count = BYTES_2_PAGES(size);
	if((pages + count) * PAGE_SIZE > KERNEL_HEAP_SIZE)
		return false;
	paging_map(KERNEL_HEAP_START + pages * PAGE_SIZE,NULL,count,PG_WRITABLE | PG_SUPERVISOR,false);

	/* take one area from the freelist and put the memory in it */
	area = kheap_allocArea();
	if(area == NULL)
		return false;

	area->address = (void*)(KERNEL_HEAP_START + pages * PAGE_SIZE);
	area->size = PAGE_SIZE * count;
	/* put area in the usable-list */
	area->next = usableList;
	usableList = area;

	pages += count;
	return true;
}

static bool kheap_loadNewAreas(void) {
	sMemArea *area,*end;

	if((pages + 1) * PAGE_SIZE > KERNEL_HEAP_SIZE)
		return false;

	/* allocate one page for area-structs */
	paging_map(KERNEL_HEAP_START + pages * PAGE_SIZE,NULL,1,PG_WRITABLE | PG_SUPERVISOR,false);

	/* determine start- and end-address */
	area = (sMemArea*)(KERNEL_HEAP_START + pages * PAGE_SIZE);
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


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void kheap_dbg_print(void) {
	sMemArea *area;
	u32 i;

	vid_printf("UsableList:\n");
	area = usableList;
	while(area != NULL) {
		vid_printf("\t0x%x: addr=0x%x, size=0x%x, next=0x%x\n",area,area->address,area->size,area->next);
		area = area->next;
	}

	/*vid_printf("FreeList:\n");
	area = freeList;
	while(area != NULL) {
		vid_printf("\t0x%x: addr=0x%x, size=0x%x, next=0x%x\n",area,area->address,area->size,area->next);
		area = area->next;
	}*/

	vid_printf("OccupiedMap:\n");
	for(i = 0; i < OCC_MAP_SIZE; i++) {
		area = occupiedMap[i];
		if(area != NULL) {
			vid_printf("\t%d:\n",i);
			while(area != NULL) {
				vid_printf("\t\t0x%x: addr=0x%x, size=0x%x, next=0x%x\n",area,area->address,area->size,area->next);
				area = area->next;
			}
		}
	}
}

#endif
