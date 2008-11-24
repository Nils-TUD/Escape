/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVKHEAP_H_
#define PRIVKHEAP_H_

#include "../pub/common.h"
#include "../pub/kheap.h"

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

#endif /* PRIVKHEAP_H_ */
