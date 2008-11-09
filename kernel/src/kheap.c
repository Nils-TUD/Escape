/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/kheap.h"
#include "../h/common.h"
#include "../h/paging.h"
#include "../h/util.h"
#include "../h/video.h"

/* TODO this does not work since we would assign new page-tables just to the current process! */
/* perhaps we should create all page-tables for 0xC0000000 - 0xFFFFFFFF at the beginning? */
/* hmm...or is it not necessary to share the kernel-heap between processes ? */

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

/**
 * Finds a place for a new mem-area
 *
 * @return the address
 */
static tMemArea *kheap_newArea(void) {
	/* TODO does it make sense to introduce a cache for some unused areas? */
	DBG_KMALLOC(vid_printf("Getting new mem-area...\n"));
	/* start-point (skip page-usage-count and initial area) */
	tMemArea *area = (tMemArea*)KERNEL_HEAP_START + 2;
	/* TODO may this loop cause trouble? */
	while(1) {
		DBG_KMALLOC(vid_printf("Testing area 0x%x\n",area));
		/* area not in use? */
		if(area->next == NULL) {
			DBG_KMALLOC(vid_printf("FOUND, breaking here\n"));
			/* increase usage-count for this page */
			u32 *usageCount = (u32*)((u32)area & ~(PAGE_SIZE - 1));
			*usageCount = *usageCount + 1;
			/* decrease total mem */
			if(area > highestMemArea) {
				initial->size = initial->size >= sizeof(tMemArea) ?
						initial->size - sizeof(tMemArea) : 0;
				highestMemArea = area;
			}
			break;
		}
		area++;

		/* reached new page? */
		if(((u32)area & (PAGE_SIZE - 1)) == 0) {
			if(highestMemArea < area) {
				DBG_KMALLOC(vid_printf("Reached new page 0x%x\n",area));
				/* get frame and map it */
				u32 frame = mm_allocateFrame(MM_DEF);
				paging_map((u32)area,&frame,1,PG_WRITABLE | PG_SUPERVISOR);
				/* we have to clear the area-pages */
				memset(area,0,PT_ENTRY_COUNT);
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
	u32 *usageCount = (u32*)((u32)area & ~(PAGE_SIZE - 1));
	/* mark slot as free */
	area->next = NULL;

	/* adjust highest-mem-area if necessary */
	if(area == highestMemArea) {
		u32 byteCount = 0;
		do {
			area--;
			byteCount += sizeof(tMemArea);
			/* skip usage-count */
			if(((u32)area & (PAGE_SIZE - 1)) == 0)
				area--;
		}
		while(area->next == NULL && area > initial);
		highestMemArea = area;

		/* add the free memory to initial area */
		initial->size += byteCount;
	}

	/* decrease usages and free frame if possible */
	*usageCount = *usageCount - 1;
	if(*usageCount == 0) {
		tPTEntry *pt = (tPTEntry*)ADDR_TO_MAPPED((u32)usageCount);
		DBG_KMALLOC(vid_printf("usageCount=0, freeing frame 0x%x @ 0x%x\n",pt->frameNumber,usageCount));
		mm_freeFrame(pt->frameNumber,MM_DEF);
		pt->present = false;
	}
}

void kheap_init(void) {
	/* TODO move that to somewhere else! */
	/* create all page-tables for this area */
	u32 addr = KERNEL_HEAP_START;
	tPTEntry *mapPageTable = (tPTEntry*)ADDR_TO_MAPPED(MAPPED_PTS_START);
	tPDEntry *pd = (tPDEntry*)PAGE_DIR_AREA + ADDR_TO_PDINDEX(addr);
	while(addr < KERNEL_HEAP_START + KERNEL_HEAP_SIZE) {
		/* insert into page-dir and map it */
		pd->present = true;
		pd->writable = true;
		pd->ptFrameNo = mm_allocateFrame(MM_DEF);
		paging_mapPageTable(mapPageTable,addr,pd->ptFrameNo,true);
		/* clear */
		memset((void*)ADDR_TO_MAPPED(addr),0,PT_ENTRY_COUNT);
		/* to next */
		addr += PAGE_SIZE * PT_ENTRY_COUNT;
		pd++;
	}

	/* get frame and map it */
	initial = (tMemArea*)KERNEL_HEAP_START + 1;
	u32 frame = mm_allocateFrame(MM_DEF);
	paging_map((u32)initial,&frame,1,PG_WRITABLE | PG_SUPERVISOR);
	/* we have to clear the area-pages */
	memset(initial - 1,0,PT_ENTRY_COUNT);

	*(u32*)KERNEL_HEAP_START = 1; /* 1 usage atm */

	/* setup initial area */
	initial->free = true;
	initial->size = KERNEL_HEAP_SIZE - sizeof(tMemArea) * 2;
	initial->next = NULL;
	first = initial;
	highestMemArea = initial;
}

u32 kheap_getFreeMem(void) {
	u32 free = 0;
	tMemArea *area = first;
	while(area != NULL) {
		if(area->free)
			free += area->size;
		area = area->next;
	}
	return free;
}

void kheap_print(void) {
	u32 address = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	tMemArea *area = first;
	vid_printf("Kernel-Heap:\n");
	do {
		vid_printf("\t(@0x%08x) 0x%08x .. 0x%08x (%d bytes) [%s]\n",area,address - area->size,
				address - 1,area->size,area->free ? "free" : "occupied");
		address -= area->size;
		area = area->next;
	}
	while(area != NULL);
}

void *kheap_alloc(u32 size) {
	u32 address,caddress,endAddr;
	tMemArea *area, *lastArea = NULL, *nArea;
	tPTEntry *pt;

	DBG_KMALLOC(vid_printf(">>===== kheap_alloc(size=%d) =====\n",size));
	DBG_KMALLOC(vid_printf("totalSize=%d, first=0x%x\n",lowestAddr,first));

	/* start at the heap-end */
	address = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;
	area = first;
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

	/* at first reserve mem for a new area (may adjust the total available mem) */
	if(area->size > size)
		nArea = kheap_newArea();

	/* not enough mem? */
	if(area->size < size)
		panic("Not enough memory to allocate %d bytes!",size);
	/* do we need a new area? */
	if(area->next == NULL)
		address -= size;
	/* otherwise start at the top */
	else
		address += area->size - size;

	/* reserve frames, if necessary */
	caddress = address;
	endAddr = caddress + size;
	while(caddress < endAddr) {
		DBG_KMALLOC(vid_printf("caddress=0x%x\n",caddress));
		pt = (tPTEntry*)ADDR_TO_MAPPED(caddress & ~(PAGE_SIZE - 1));
		if(!pt->present) {
			u32 frame = mm_allocateFrame(MM_DEF);
			DBG_KMALLOC(vid_printf("Allocating frame for 0x%x -> 0x%x\n",caddress,frame));
			pt->present = true;
			pt->writable = true;
			pt->notSuperVisor = false;
			pt->frameNumber = frame;
		}
		caddress += PAGE_SIZE;
	}

	DBG_KMALLOC(vid_printf("OldArea(@0x%x): free=%d, size=%d, next=0x%x\n",area,area->free,
			area->size,area->next));
	/* split area? */
	if(area->size > size) {
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
		/* initial area has to be free! */
		if(area == initial)
			panic("You can't occupie the initial area completely! => not enough mem\n");

		area->free = false;
	}

	DBG_KMALLOC(vid_printf("OldArea(@0x%x): free=%d, size=%d, next=0x%x\n",area,area->free,
			area->size,area->next));
	DBG_KMALLOC(vid_printf("<<===== kheap_alloc =====\n"));

	/* return address of the area */
	return (void*)address;
}

void kheap_free(void *addr) {
	u32 address, freeSize, startAddr, lastAddr;
	tMemArea *area, *lastArea, *lastLastArea, *next;

	DBG_KMALLOC(vid_printf(">>===== kheap_free(addr=0x%x) =====\n",addr));
	DBG_KMALLOC(vid_printf("totalSize=%d, first=0x%x\n",lowestAddr,first));

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

	startAddr = address;
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
				startAddr -= next->size;
				freeSize += next->size;
			}
			else {
				/* next is initial area, so we can free the frame */
				startAddr -= PAGE_SIZE;
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

	address = (startAddr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	/* do not delete our area-structures */
	if(address == ((u32)highestMemArea & ~(PAGE_SIZE - 1)))
		address += PAGE_SIZE;
	/* don't free to much */
	freeSize = address - startAddr > freeSize ? 0 : freeSize - (address - startAddr);
	freeSize &= ~(PAGE_SIZE - 1);
	DBG_KMALLOC(vid_printf("After: address=0x%x, totalSize=%d\n",address,freeSize));
	while(freeSize > 0) {
		DBG_KMALLOC(vid_printf("address=0x%x, totalSize=%d\n",address,freeSize));
		tPTEntry *pt = (tPTEntry*)ADDR_TO_MAPPED(address);
		/* not already free'd? */
		if(pt->present) {
			DBG_KMALLOC(vid_printf("Freeing frame %x\n",pt->frameNumber));
			mm_freeFrame(pt->frameNumber,MM_DEF);
			/* Note that we don't remove the page-table! */
			pt->present = false;
		}
		/* to next */
		freeSize -= PAGE_SIZE;
		address += PAGE_SIZE;
	}

	DBG_KMALLOC(vid_printf("<<===== kheap_free =====\n"));
}

#ifdef TEST_KHEAP

#define TEST_COUNT (1 + ARRAY_SIZE(sizes) + 1 + 1 + 1)
/* reach next page with mem-area-structs */
#define SINGLE_BYTE_COUNT 513
u32 *ptrsSingle[SINGLE_BYTE_COUNT];

u32 oldPC, newPC;
u32 oldFC, newFC;
u32 oldFH, newFH;
u32 success = 0;

static void test_init(void) {
	oldPC = paging_getPageCount();
	oldFC = mm_getNumberOfFreeFrames(MM_DEF);
	oldFH = kheap_getFreeMem();
}

static void test_check(void) {
	newPC = paging_getPageCount();
	newFC = mm_getNumberOfFreeFrames(MM_DEF);
	newFH = kheap_getFreeMem();
	if(newPC != oldPC || newFC != oldFC || newFH != oldFH) {
		vid_printf("FAILED: old-page-count=%d, new-page-count=%d,"
				" old-frame-count=%d, new-frame-count=%d, old-heap-count=%d, new-heap-count=%d\n",
				oldPC,newPC,oldFC,newFC,oldFH,newFH);
	}
	else {
		success++;
		vid_printf("SUCCESS\n");
	}
}

static bool test_checkContent(u32 *ptr,u32 count,u32 value) {
	while(count-- > 0) {
		if(*ptr++ != value)
			return false;
	}
	return true;
}

void test_kheap(void) {
	u32 freeAlgos[] = {0,1,2,3};
	u32 randFree1[] = {7,5,2,0,6,3,4,1};
	u32 randFree2[] = {3,4,1,5,6,0,7,2};
	u32 sizes[] = {1,4,10,1023,1024,1025,2048,4097};
	u32 *ptrs[ARRAY_SIZE(sizes)];
	u32 freeAlgo;
	s32 size;

	vid_printf("Starting first kheap-test...\n");
	vid_printf("pageCount=%d, frameCount=%d\n",oldPC,oldFC);

	test_init();

	/* allocate all, then free */
	for(freeAlgo = 0; freeAlgo < ARRAY_SIZE(freeAlgos); freeAlgo++) {
		vid_printf("Allocating...(%d free frames)\n",mm_getNumberOfFreeFrames(MM_DEF));
		for(size = 0; (u32)size < ARRAY_SIZE(sizes); size++) {
			vid_printf("%d bytes\n",sizes[size] * sizeof(u32));
			ptrs[size] = (u32*)kheap_alloc(sizes[size] * sizeof(u32));
			vid_printf("Write test for 0x%x...",ptrs[size]);
			/* write test */
			*(ptrs[size]) = 4;
			*(ptrs[size] + sizes[size] - 1) = 2;
			vid_printf("done\n");
		}

		vid_printf("\nFreeing...\n");
		switch(freeAlgo) {
			case 0:
				for(size = 0; (u32)size < ARRAY_SIZE(sizes); size++) {
					vid_printf("FREE1: address=0x%x, i=%d\n",ptrs[size],size);
					kheap_free(ptrs[size]);
					/* write test */
					u32 i;
					for(i = size + 1; i < ARRAY_SIZE(sizes); i++) {
						*(ptrs[i]) = 1;
						*(ptrs[i] + sizes[i] - 1) = 2;
					}
				}
				break;
			case 1:
				for(size = ARRAY_SIZE(sizes) - 1; size >= 0; size--) {
					vid_printf("FREE2: address=0x%x, i=%d\n",ptrs[size],size);
					kheap_free(ptrs[size]);
					/* write test */
					s32 i;
					for(i = size - 1; i >= 0; i--) {
						*(ptrs[i]) = 1;
						*(ptrs[i] + sizes[i] - 1) = 2;
					}
				}
				break;
			case 2:
				for(size = 0; (u32)size < ARRAY_SIZE(sizes); size++) {
					vid_printf("FREE3: address=0x%x, i=%d\n",ptrs[randFree1[size]],size);
					kheap_free(ptrs[randFree1[size]]);
				}
				break;
			case 3:
				for(size = 0; (u32)size < ARRAY_SIZE(sizes); size++) {
					vid_printf("FREE4: address=0x%x, i=%d\n",ptrs[randFree2[size]],size);
					kheap_free(ptrs[randFree2[size]]);
				}
				break;
		}
		vid_printf("\n");
	}

	test_check();

	vid_printf("\n----------------------------\n");
	vid_printf("Starting second kheap-test...\n");

	/* free directly */
	for(size = 0; (u32)size < ARRAY_SIZE(sizes); size++) {
		test_init();

		vid_printf("Allocating %d bytes\n",sizes[size] * sizeof(u32));

		ptrs[0] = (u32*)kheap_alloc(sizes[size] * sizeof(u32));
		vid_printf("Write test for 0x%x...",ptrs[0]);
		/* write test */
		*(ptrs[0]) = 1;
		*(ptrs[0] + sizes[size] - 1) = 2;
		vid_printf("done\n");
		vid_printf("Freeing mem @ 0x%x\n",ptrs[0]);
		kheap_free(ptrs[0]);

		test_check();
	}


	/* allocate single bytes to reach the next page for the mem-area-structs */
	vid_printf("\n----------------------------\n");
	vid_printf("Starting third kheap-test...\n");
	test_init();
	vid_printf("Allocating %d times 1 byte...\n",SINGLE_BYTE_COUNT);
	u32 i;
	for(i = 0; i < SINGLE_BYTE_COUNT; i++) {
		ptrsSingle[i] = kheap_alloc(1);
	}
	vid_printf("Freeing...\n");
	for(i = 0; i < SINGLE_BYTE_COUNT; i++) {
		kheap_free(ptrsSingle[i]);
	}
	vid_printf("\n");
	test_check();


	/* allocate all */
	vid_printf("\n----------------------------\n");
	vid_printf("Starting fourth kheap-test...\n");
	test_init();
	vid_printf("Allocating all...\n");
	u32 *ptr = kheap_alloc(KERNEL_HEAP_SIZE - sizeof(tMemArea) * 3 - 1);
	vid_printf("free it...\n");
	kheap_free(ptr);
	test_check();


	/* reallocate test */
	vid_printf("\n----------------------------\n");
	vid_printf("Starting fifth kheap-test...\n");
	u32 *ptr1,*ptr2,*ptr3,*ptr4,*ptr5;
	vid_printf("Allocating 3 regions...\n");
	ptr1 = kheap_alloc(4 * sizeof(u32));
	for(i = 0;i < 4;i++)
		*(ptr1+i) = 1;
	ptr2 = kheap_alloc(8 * sizeof(u32));
	for(i = 0;i < 8;i++)
		*(ptr2+i) = 2;
	ptr3 = kheap_alloc(12 * sizeof(u32));
	for(i = 0;i < 12;i++)
		*(ptr3+i) = 3;
	vid_printf("Freeing region 2...\n");
	kheap_free(ptr2);
	vid_printf("Reusing region 2...\n");
	ptr4 = kheap_alloc(6 * sizeof(u32));
	for(i = 0;i < 6;i++)
		*(ptr4+i) = 4;
	ptr5 = kheap_alloc(2 * sizeof(u32));
	for(i = 0;i < 2;i++)
		*(ptr5+i) = 5;

	vid_printf("Testing contents...\n");
	if(test_checkContent(ptr1,4,1) &&
			test_checkContent(ptr3,12,3) &&
			test_checkContent(ptr4,6,4) &&
			test_checkContent(ptr5,2,5)) {
		vid_printf("SUCCESS\n");
		success++;
	}
	else {
		vid_printf("FAILED\n");
	}

	/* print total results */
	vid_printf("\n----------------------------\n");
	vid_printf("%d tests successfull\n",success);
	vid_printf("%d tests failed\n",TEST_COUNT - success);
	vid_printf("----------------------------\n");
}

#endif
