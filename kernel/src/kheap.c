/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/kheap.h"
#include "../h/common.h"
#include "../h/paging.h"
#include "../h/util.h"
#include <video.h>
#include <string.h>

/*
 * Consider the following actions:
 * 	ptr1 = kheap_alloc(16);
 *  ptr2 = kheap_alloc(29);
 *  ptr3 = kheap_alloc(15);
 *  kheap_free(ptr2);
 *  ptr2 = kheap_alloc(14);
 *  kheap_free(ptr3);
 *  kheap_free(ptr1);
 *
 * That would lead to the following situation:
 * Kernel-Heap:
 *   (@0xC1800010) 0xC27FFFF0 .. 0xC27FFFFF (16 bytes) [free]
 *   (@0xC1800028) 0xC27FFFE2 .. 0xC27FFFEF (14 bytes) [occupied]
 *   (@0xC1800008) 0xC1800030 .. 0xC27FFFE1 (16777138 bytes) [free]
 *
 * Memory-view:
 *    +    +--------------+--------------+  @ 0xC1800000 = KERNEL_HEAP_START
 *    |    | usageCount=5 |       0      |                  <- usageCount on each page
 *    |    +---+----------+--------------+ <--------\       <- initial
 *    |    |f=1|  s=<all> |  next(NULL)  |          |          (<all> = remaining heapmem)
 *    |    +---+----------+--------------+          |       <- first
 *    |    |f=1|   s=16   |     next     | ---\     |
 *   mem   +---+----------+--------------+    |     |
 *  areas  |f=?|   s=?    |  next(NULL)  |    |     |
 *    |    +---+----------+--------------+    |     |
 *    |    |f=?|   s=?    |  next(NULL)  |    |     |
 *    |    +---+----------+--------------+ <--/     |       <- highessMemArea
 *    |    |f=0|   s=14   |     next     | ---------/
 *    .    +---+----------+--------------+  @ 0xC1800030
 *    .    |             ...             |
 *    v    +-----------------------------+
 *         .                             .
 *    ^    .                             .
 *    .    .                             .
 *    .    +-----------------------------+  @ 0xC27FFFE2
 *    |    |           14 bytes          |
 *  data   +-----------------------------+  @ 0xC27FFFF0
 *    |    |       16 bytes (free)       |
 *    +    +-----------------------------+  @ 0xC2800000 = KERNEL_HEAP_START + KERNEL_HEAP_SIZE
 */

/*
 * TODO would the following mem-area-concept be better:
 *  - declare a max. number of mem-areas and reserve the memory for them
 *  - introduce a free-list which will be filled with all areas at the beginning
 * That means we can get and delete areas with 0(1).
 */

/* represents one area of memory which may be free or occupied */
typedef struct sMemArea sMemArea;
struct sMemArea {
	/* wether this area is free */
	u32 free				: 1,
	/* the size of this area in bytes */
		size				: 31;
	/* the pointer to the next area; NULL if there is no */
	sMemArea *next;
};

/**
 * Intern free function. Frees the given area with given address. You have to provide the
 * last and last-last area, too.
 *
 * @param address the address of the area
 * @param area the area to free
 * @param lastArea the previous of area
 * @param lastLastArea the previous of the previous area
 */
static void kheap_freeIntern(u32 address,sMemArea *area,sMemArea *lastArea,sMemArea *lastLastArea);

/**
 * Finds a place for a new mem-area
 *
 * @param size the desired size
 * @param isInitial wether the initial area should be splitted
 * @return the address or NULL if there is not enough mem
 */
static sMemArea *kheap_newArea(u32 size,bool isInitial);

/**
 * Deletes the given area
 *
 * @param area the area
 */
static void kheap_deleteArea(sMemArea *area);

/* our initial area which contains the remaining free mem */
static sMemArea *initial;
/* the beginning of the list */
static sMemArea *first;
/* the "highest" (the address) mem-area */
static sMemArea *highessMemArea;

/* the area to start searching for free areas */
static u32 startAddr;
static sMemArea *startArea;
static sMemArea *startPrev;

/* the first unused area */
static sMemArea *firstUnused;

void kheap_init(void) {
	/* map frame for the initial area */
	initial = (sMemArea*)KERNEL_HEAP_START + 1;
	paging_map((u32)initial,NULL,1,PG_WRITABLE | PG_SUPERVISOR,false);
	/* we have to clear the area-pages */
	memset(initial - 1,0,PAGE_SIZE);

	*(u32*)KERNEL_HEAP_START = 1; /* 1 usage atm */

	/* setup initial area */
	initial->free = true;
	initial->size = KERNEL_HEAP_SIZE - PAGE_SIZE;
	initial->next = NULL;
	first = initial;
	highessMemArea = initial;

	/* start-settings for kheap_alloc() */
	startAddr = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	startPrev = NULL;
	startArea = first;

	/* first unused area */
	firstUnused = (sMemArea*)KERNEL_HEAP_START + 2;
}

u32 kheap_getFreeMem(void) {
	u32 free = 0;
	sMemArea *area = first;
	while(area != NULL) {
		if(area->free)
			free += area->size;
		area = area->next;
	}
	/* the last byte can't be used */
	return free - 1;
}

void *kheap_alloc(u32 size) {
	u32 address;
	sMemArea *area, *lastArea, *nArea;

	ASSERT(size > 0,"size == 0");

	DBG_KMALLOC(vid_printf(">>===== kheap_alloc(size=%d) =====\n",size));
	DBG_KMALLOC(vid_printf("firstUnused=0x%x, first=0x%x\n",firstUnused,first));
	DBG_KMALLOC(vid_printf("startAddr=0x%x, startPrev=0x%x, startArea=0x%x\n",
			startAddr,startPrev,startArea));
	/*kheap_print();*/

	/* start at the first area that may be free */
	address = startAddr;
	lastArea = startPrev;
	area = startArea;
	while(area->next != NULL) {
		DBG_KMALLOC(vid_printf("address=0x%x, area=0x%x, free=%d, size=%d, next=0x%x\n",address,
				area,area->free,area->size,area->next));
		address -= area->size;
		if(area->free && area->size >= size)
			break;
		lastArea = area;
		area = area->next;
	}

	DBG_KMALLOC(vid_printf("FINALLY: address=0x%x, area=0x%x, free=%d, size=%d, next=0x%x\n",
			address,area,area->free,area->size,area->next));

	/* not enough mem? */
	if(area->size < size)
		return NULL;
	/* initial area has to be free! */
	if(area->size == size && area == initial)
		return NULL;

	/* at first reserve mem for a new area (may adjust the total available mem) */
	if(area->size > size) {
		nArea = kheap_newArea(size,area == initial);
		if(nArea == NULL)
			return NULL;

		/* now everything has to be ok because kheap_newArea() might have changed our state! */
	}

	/* do we need a new area? */
	if(area->next == NULL)
		address -= size;
	/* otherwise start at the top */
	else
		address += area->size - size;

	/* reserve frames and map them, if necessary */
	paging_map(address,NULL,BYTES_2_PAGES((address & (PAGE_SIZE - 1)) + size),PG_SUPERVISOR | PG_WRITABLE,false);

	DBG_KMALLOC(vid_printf("OldArea(@0x%x): free=%d, size=%d, next=0x%x\n",area,area->free,
			area->size,area->next));
	/* split area? */
	if(area->size > size) {
		/* adjust start */
		startAddr = address;
		startArea = area;
		startPrev = nArea;

		nArea->size = size;
		nArea->free = false;
		nArea->next = area;
		area->size -= size;
		if(lastArea != NULL)
			lastArea->next = nArea;
		else
			first = nArea;
		DBG_KMALLOC(vid_printf("NewArea(@0x%x): free=%d, size=%d, next=0x%x\n",nArea,nArea->free,
				nArea->size,nArea->next));
	}
	/* fits exactly */
	else {
		/* adjust start */
		startAddr = address;
		startArea = area->next;
		startPrev = area;

		area->free = false;
	}

	DBG_KMALLOC(vid_printf("OldArea(@0x%x): free=%d, size=%d, next=0x%x\n",area,area->free,
			area->size,area->next));
	DBG_KMALLOC(vid_printf("<<===== kheap_alloc =====\n"));

	/* return address of the area */
	return (void*)address;
}

void kheap_free(void *addr) {
	u32 address;
	sMemArea *area, *lastArea, *lastLastArea;

	ASSERT(addr != NULL,"addr == NULL");

	DBG_KMALLOC(vid_printf(">>===== kheap_free(addr=0x%x) =====\n",addr));
	DBG_KMALLOC(vid_printf("firstUnused=0x%x, first=0x%x\n",firstUnused,first));
	DBG_KMALLOC(vid_printf("startAddr=0x%x, startPrev=0x%x, startArea=0x%x\n",
			startAddr,startPrev,startArea));

	/* search the matching area */
	address = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	area = first;
	lastArea = NULL, lastLastArea = NULL;
	while(area != NULL) {
		address -= area->size;
		/* area found? */
		if((void*)address == addr)
			break;

		lastLastArea = lastArea;
		lastArea = area;
		area = area->next;
	}

	kheap_freeIntern(address,area,lastArea,lastLastArea);

	DBG_KMALLOC(vid_printf("<<===== kheap_free =====\n"));
}

void *kheap_realloc(void *addr,u32 size) {
	u32 address, newAddress;
	sMemArea *area, *lastArea, *lastLastArea;

	/* search the matching area */
	address = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	area = first;
	lastArea = NULL, lastLastArea = NULL;
	while(area != NULL) {
		address -= area->size;
		/* area found? */
		if((void*)address == addr)
			break;

		lastLastArea = lastArea;
		lastArea = area;
		area = area->next;
	}

	/* ignore shrinks */
	/* TODO keep that? */
	if(size < area->size)
		return (void*)address;

	/* if the prev area is not big enough or occupied we need a new one */
	if(lastArea == NULL || !lastArea->free || lastArea->size < size - area->size) {
		/*vid_printf("BEFORE:\n");
		paging_dbg_printPageDir(true);*/

		/* get new area */
		newAddress = (u32)kheap_alloc(size);
		if((void*)newAddress == NULL)
			return NULL;

		/* copy data */
		memcpy((void*)newAddress,(void*)address,area->size);

		/* free old area */
		/*kheap_freeIntern(address,area,lastArea,lastLastArea);*/
		kheap_free((void*)address);
		/*vid_printf("AFTER:\n");
		paging_dbg_printPageDir(true);*/
		return (void*)newAddress;
	}

	/* ok, the prev is enough, so we have to add it to our area */

	/* do we need the complete prev area? */
	if(lastArea->size == size - area->size) {
		/* remove lastArea */
		if(lastLastArea != NULL)
			lastLastArea->next = area;
		else
			first = area;
		kheap_deleteArea(lastArea);
		/* increase size */
		area->size = size;
	}
	/* otherwise simply change sizes */
	else {
		lastArea->size -= size - area->size;
		area->size = size;
	}

	return (void*)address;
}

static void kheap_freeIntern(u32 address,sMemArea *area,sMemArea *lastArea,sMemArea *lastLastArea) {
	u32 freeSize, lstartAddr;
	sMemArea *next;

	/* check if area is valid */
	ASSERT(area != NULL,"MemArea for address 0x%08x doesn't exist!",address);
	ASSERT(!area->free,"Duplicate free of address 0x%08x!",address);

	DBG_KMALLOC(vid_printf("area=0x%x, area->free=%d, area->size=%d, area->next=0x%x\n",
			area,area->free,area->size,area->next));

	lstartAddr = address;
	freeSize = area->size;
	area->free = true;

	/* check prev */
	if(lastArea != NULL && lastArea->free) {
		/* adjust free settings */
		freeSize += lastArea->size;
		/* add to last one */
		lastArea->size += area->size;
		lastArea->next = area->next;
		kheap_deleteArea(area);
		area = lastArea;
		lastArea = lastLastArea;
	}
	/* check next (again, because area may have changed) */
	if((next = area->next) != NULL) {
		if(next->free) {
			/* adjust free settings (not for the last area, because it's free) */
			if(next->next != NULL) {
				lstartAddr -= next->size;
				freeSize += next->size;
			}
			else {
				/* next is initial area, so we can free the frame */
				lstartAddr -= PAGE_SIZE;
				freeSize += PAGE_SIZE;
			}

			/* special case to prevent that the initial-area will be replaced with something else */
			if(next == initial) {
				if(lastArea == NULL)
					first = initial;
				else
					lastArea->next = initial;
				initial->size += area->size;
				kheap_deleteArea(area);
			}
			else {
				/* add to this one */
				area->size += next->size;
				area->next = next->next;
				kheap_deleteArea(next);
			}
		}
	}

	/* ensure that we can free the page (go to next page-start) */
	DBG_KMALLOC(vid_printf("Before: startAddr=0x%x, totalSize=%d\n",startAddr,freeSize));

	address = (lstartAddr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	/* don't free to much */
	freeSize = address - lstartAddr > freeSize ? 0 : freeSize - (address - lstartAddr);
	freeSize &= ~(PAGE_SIZE - 1);

	paging_unmap(address,BYTES_2_PAGES(freeSize),true);

	/* adjust start for kheap_alloc() */
	/* TODO improve that? */
	startArea = first;
	startAddr = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	startPrev = NULL;
}

static sMemArea *kheap_newArea(u32 size,bool isInitial) {
	DBG_KMALLOC(vid_printf("Getting new mem-area...\n"));
	/* start-point (skip page-usage-count and initial area) */
	sMemArea *area = firstUnused;
	/* TODO may this loop cause trouble? */
	while(1) {
		DBG_KMALLOC(vid_printf("Testing area 0x%x\n",area));
		/* area not in use? */
		if(area->next == NULL) {
			DBG_KMALLOC(vid_printf("FOUND, breaking here\n"));
			/* increase usage-count for this page */
			u32 *usageCount = (u32*)((u32)area & ~(PAGE_SIZE - 1));
			*usageCount = *usageCount + 1;
			/* set highessMemArea */
			if(area > highessMemArea)
				highessMemArea = area;
			/* prevent a page-fault */
			if(((u32)area & (PAGE_SIZE - 1)) == PAGE_SIZE - sizeof(sMemArea))
				firstUnused = area;
			else
				firstUnused = area + 1;
			break;
		}
		area++;

		/* reached new page? */
		if(((u32)area & (PAGE_SIZE - 1)) == 0) {
			if(highessMemArea < area) {
				/* not enough mem? */
				if(initial->size < PAGE_SIZE)
					return NULL;
				if(isInitial && initial->size < PAGE_SIZE + size)
					return NULL;

				DBG_KMALLOC(vid_printf("Reached new page 0x%x\n",area));
				/* get frame and map it */
				u32 frame = mm_allocateFrame(MM_DEF);
				paging_map((u32)area,&frame,1,PG_WRITABLE | PG_SUPERVISOR,false);
				/* we have to clear the area-pages */
				memset(area,0,PAGE_SIZE);
				/* reduce available mem */
				initial->size -= PAGE_SIZE;
			}
			/* skip page-usage-count */
			area++;
		}
	}
	return area;
}

static void kheap_deleteArea(sMemArea *area) {
	sMemArea *oldHighest = highessMemArea;
	u32 *usageCount = (u32*)((u32)area & ~(PAGE_SIZE - 1));

	ASSERT(area != NULL,"area is NULL");

	/* mark slot as free */
	area->next = NULL;

	if(area < firstUnused)
		firstUnused = area;

	/* adjust highest-mem-area if necessary */
	if(area == highessMemArea) {
		do {
			area--;
			/* skip usage-count */
			if(((u32)area & (PAGE_SIZE - 1)) == 0) {
				area--;
				/* skip not mapped pages */
				while(!paging_isMapped((u32)area))
					area -= PAGE_SIZE / sizeof(sMemArea);
			}
		}
		while(area->next == NULL && area > initial);
		highessMemArea = area;
	}

	/* decrease usages and free frame if possible */
	*usageCount = *usageCount - 1;
	if(*usageCount == 0) {
		DBG_KMALLOC(vid_printf("usageCount=0, freeing frame @ 0x%x\n",usageCount));
		paging_unmap((u32)usageCount,1,true);
		/* TODO we can choose a better one, right? */
		firstUnused = (sMemArea*)KERNEL_HEAP_START + 2;
		/* TODO optimize (required here because we test if a page is mapped) */
		paging_flushTLB();

		/* give the memory back */
		initial->size += ((u32)oldHighest & ~(PAGE_SIZE - 1))
			- ((u32)highessMemArea & ~(PAGE_SIZE - 1));
	}
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void kheap_dbg_print(void) {
	u32 address = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	sMemArea *area = first;
	vid_printf("Kernel-Heap (first=0x%x, highessMemArea=0x%x, startArea=0x%x, startAddr=0x%x, "
			"startPrev=0x%x, firstUnused=0x%x):\n",first,highessMemArea,startArea,startAddr,
			startPrev,firstUnused);
	do {
		vid_printf("\t(@0x%08x) 0x%08x .. 0x%08x (%d bytes) [%s]\n",area,address - area->size,
				address - 1,area->size,area->free ? "free" : "occupied");
		address -= area->size;
		area = area->next;
	}
	while(area != NULL);
}

#endif
