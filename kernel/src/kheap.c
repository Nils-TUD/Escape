/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/kheap.h"
#include "../h/common.h"
#include "../h/paging.h"
#include "../h/util.h"
#include "../h/string.h"
#include "../h/video.h"

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
 *    |    +---+----------+--------------+ <--/     |       <- highestMemArea
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
typedef struct tMemArea tMemArea;
struct tMemArea {
	/* wether this area is free */
	u32 free				: 1,
	/* the size of this area in bytes */
		size				: 31;
	/* the pointer to the next area; NULL if there is no */
	tMemArea *next;
};

/* our initial area which contains the remaining free mem */
static tMemArea *initial;
/* the beginning of the list */
static tMemArea *first;
/* the "highest" (the address) mem-area */
static tMemArea *highestMemArea;

/* the area to start searching for free areas */
static u32 startAddr;
static tMemArea *startArea;
static tMemArea *startPrev;

/* the first unused area */
static tMemArea *firstUnused;

/**
 * Finds a place for a new mem-area
 *
 * @param size the desired size
 * @param isInitial wether the initial area should be splitted
 * @return the address or NULL if there is not enough mem
 */
static tMemArea *kheap_newArea(u32 size,bool isInitial) {
	DBG_KMALLOC(vid_printf("Getting new mem-area...\n"));
	/* start-point (skip page-usage-count and initial area) */
	tMemArea *area = firstUnused;
	/* TODO may this loop cause trouble? */
	while(1) {
		DBG_KMALLOC(vid_printf("Testing area 0x%x\n",area));
		/* area not in use? */
		if(area->next == NULL) {
			DBG_KMALLOC(vid_printf("FOUND, breaking here\n"));
			/* increase usage-count for this page */
			u32 *usageCount = (u32*)((u32)area & ~(PAGE_SIZE - 1));
			*usageCount = *usageCount + 1;
			/* set highestMemArea */
			if(area > highestMemArea)
				highestMemArea = area;
			/* prevent a page-fault */
			if(((u32)area & (PAGE_SIZE - 1)) == PAGE_SIZE - sizeof(tMemArea))
				firstUnused = area;
			else
				firstUnused = area + 1;
			break;
		}
		area++;

		/* reached new page? */
		if(((u32)area & (PAGE_SIZE - 1)) == 0) {
			if(highestMemArea < area) {
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

/**
 * Deletes the given area
 *
 * @param area the area
 */
static void kheap_deleteArea(tMemArea *area) {
	tMemArea *oldHighest = highestMemArea;
	u32 *usageCount = (u32*)((u32)area & ~(PAGE_SIZE - 1));
	/* mark slot as free */
	area->next = NULL;

	if(area < firstUnused)
		firstUnused = area;

	/* adjust highest-mem-area if necessary */
	if(area == highestMemArea) {
		do {
			area--;
			/* skip usage-count */
			if(((u32)area & (PAGE_SIZE - 1)) == 0) {
				area--;
				/* skip not mapped pages */
				while(!paging_isMapped((u32)area))
					area -= PAGE_SIZE / sizeof(tMemArea);
			}
		}
		while(area->next == NULL && area > initial);
		highestMemArea = area;
	}

	/* decrease usages and free frame if possible */
	*usageCount = *usageCount - 1;
	if(*usageCount == 0) {
		DBG_KMALLOC(vid_printf("usageCount=0, freeing frame @ 0x%x\n",usageCount));
		paging_unmap((u32)usageCount,1,true);
		/* TODO we can choose a better one, right? */
		firstUnused = (tMemArea*)KERNEL_HEAP_START + 2;
		/* TODO optimize (required here because we test if a page is mapped) */
		paging_flushTLB();

		/* give the memory back */
		initial->size += ((u32)oldHighest & ~(PAGE_SIZE - 1))
			- ((u32)highestMemArea & ~(PAGE_SIZE - 1));
	}
}

void kheap_init(void) {
	/* get frame and map it */
	initial = (tMemArea*)KERNEL_HEAP_START + 1;
	u32 frame = mm_allocateFrame(MM_DEF);
	paging_map((u32)initial,&frame,1,PG_WRITABLE | PG_SUPERVISOR,false);
	/* we have to clear the area-pages */
	memset(initial - 1,0,PAGE_SIZE);

	*(u32*)KERNEL_HEAP_START = 1; /* 1 usage atm */

	/* setup initial area */
	initial->free = true;
	initial->size = KERNEL_HEAP_SIZE - PAGE_SIZE;
	initial->next = NULL;
	first = initial;
	highestMemArea = initial;

	/* start-settings for kheap_alloc() */
	startAddr = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	startPrev = NULL;
	startArea = first;

	/* first unused area */
	firstUnused = (tMemArea*)KERNEL_HEAP_START + 2;
}

u32 kheap_getFreeMem(void) {
	u32 free = 0;
	tMemArea *area = first;
	while(area != NULL) {
		if(area->free)
			free += area->size;
		area = area->next;
	}
	/* the last byte can't be used */
	return free - 1;
}

void kheap_print(void) {
	u32 address = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	tMemArea *area = first;
	vid_printf("Kernel-Heap (first=0x%x, highestMemArea=0x%x, startArea=0x%x, startAddr=0x%x, "
			"startPrev=0x%x, firstUnused=0x%x):\n",first,highestMemArea,startArea,startAddr,
			startPrev,firstUnused);
	do {
		vid_printf("\t(@0x%08x) 0x%08x .. 0x%08x (%d bytes) [%s]\n",area,address - area->size,
				address - 1,area->size,area->free ? "free" : "occupied");
		address -= area->size;
		area = area->next;
	}
	while(area != NULL);
}

void *kheap_alloc(u32 size) {
	u32 address;
	tMemArea *area, *lastArea, *nArea;

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
	paging_map(address,NULL,BYTES_2_PAGES(size),PG_SUPERVISOR | PG_WRITABLE,false);

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
	u32 address, freeSize, lstartAddr, lastAddr;
	tMemArea *area, *lastArea, *lastLastArea, *next;

	DBG_KMALLOC(vid_printf(">>===== kheap_free(addr=0x%x) =====\n",addr));
	DBG_KMALLOC(vid_printf("firstUnused=0x%x, first=0x%x\n",firstUnused,first));
	DBG_KMALLOC(vid_printf("startAddr=0x%x, startPrev=0x%x, startArea=0x%x\n",
			startAddr,startPrev,startArea));
	/*kheap_print();*/

	/* search the matching area */
	address = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	lastAddr = 0;
	area = first;
	lastArea = NULL, lastLastArea = NULL;
	while(area != NULL) {
		address -= area->size;
		/* area found? */
		if((void*)address == addr)
			break;

		lastAddr = address;
		lastLastArea = lastArea;
		lastArea = area;
		area = area->next;
	}

	/* check if area is valid */
	if(area == NULL)
		panic("MemArea for address 0x%08x doesn't exist!",addr);
	if(area->free)
		panic("Duplicate free of address 0x%08x!",addr);

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

	DBG_KMALLOC(vid_printf("<<===== kheap_free =====\n"));
}
