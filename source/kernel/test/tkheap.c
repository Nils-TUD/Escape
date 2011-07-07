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
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <esc/test.h>
#include <sys/video.h>
#include <stdarg.h>
#include "tkheap.h"
#include "testutils.h"

/* forward declarations */
static void test_kheap(void);
static void test_kheap_t1v1(void);
static void test_kheap_t1v2(void);
static void test_kheap_t1v3(void);
static void test_kheap_t1v4(void);
static void test_kheap_t2(void);
static void test_kheap_t3(void);
static void test_kheap_t5(void);
static void test_kheap_realloc(void);

/* our test-module */
sTestModule tModKHeap = {
	"Kernel-Heap",
	&test_kheap
};

#define SINGLE_BYTE_COUNT 10000
uint *ptrsSingle[SINGLE_BYTE_COUNT];

size_t sizes[] = {1,4,10,1023,1024,1025,2048,4097};
uint *ptrs[ARRAY_SIZE(sizes)];
size_t randFree1[] = {7,5,2,0,6,3,4,1};
size_t randFree2[] = {3,4,1,5,6,0,7,2};

/* helper */

static bool test_checkContent(uint *ptr,size_t count,uint value) {
	while(count-- > 0) {
		if(*ptr++ != value)
			return false;
	}
	return true;
}

static void test_t1alloc(void) {
	size_t size;
	tprintf("Allocating...(%d free frames)\n",pmem_getFreeFrames(MM_DEF | MM_CONT));
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		tprintf("%d bytes\n",sizes[size] * sizeof(uint));
		ptrs[size] = (uint*)kheap_alloc(sizes[size] * sizeof(uint));
		if(ptrs[size] == NULL)
			tprintf("Not enough mem\n");
		else {
			tprintf("Write test for 0x%x...",ptrs[size]);
			/* write test */
			*(ptrs[size]) = 4;
			*(ptrs[size] + sizes[size] - 1) = 2;
			tprintf("done\n");
		}
	}
}

static void test_kheap(void) {
	void (*tests[])(void) = {
		&test_kheap_t1v1,
		&test_kheap_t1v2,
		&test_kheap_t1v3,
		&test_kheap_t1v4,
		&test_kheap_t2,
		&test_kheap_t3,
		&test_kheap_t5,
		&test_kheap_realloc
	};

	size_t i;
	for(i = 0; i < ARRAY_SIZE(tests); i++) {
		tests[i]();
	}
}

/* test functions */

/* allocate, free in same direction */
static void test_kheap_t1v1(void) {
	size_t size,i;
	test_caseStart("Allocate, then free in same direction");
	checkMemoryBefore(false);
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		if(ptrs[size] != NULL) {
			tprintf("FREE1: address=0x%x, i=%d\n",ptrs[size],size);
			kheap_free(ptrs[size]);
			/* write test */
			for(i = size + 1; i < ARRAY_SIZE(sizes); i++) {
				if(ptrs[i] != NULL) {
					*(ptrs[i]) = 1;
					*(ptrs[i] + sizes[i] - 1) = 2;
				}
			}
		}
	}
	checkMemoryAfter(false);
}

/* allocate, free in opposite direction */
static void test_kheap_t1v2(void) {
	ssize_t size,i;
	test_caseStart("Allocate, then free in opposite direction");
	checkMemoryBefore(false);
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = ARRAY_SIZE(sizes) - 1; size >= 0; size--) {
		if(ptrs[size] != NULL) {
			tprintf("FREE2: address=0x%x, i=%d\n",ptrs[size],size);
			kheap_free(ptrs[size]);
			/* write test */
			for(i = size - 1; i >= 0; i--) {
				if(ptrs[i] != NULL) {
					*(ptrs[i]) = 1;
					*(ptrs[i] + sizes[i] - 1) = 2;
				}
			}
		}
	}
	checkMemoryAfter(false);
}

/* allocate, free in random direction 1 */
static void test_kheap_t1v3(void) {
	size_t size;
	test_caseStart("Allocate, then free in \"random\" direction 1");
	checkMemoryBefore(false);
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		if(ptrs[randFree1[size]] != NULL) {
			tprintf("FREE3: address=0x%x, i=%d\n",ptrs[randFree1[size]],size);
			kheap_free(ptrs[randFree1[size]]);
		}
	}
	checkMemoryAfter(false);
}

/* allocate, free in random direction 2 */
static void test_kheap_t1v4(void) {
	size_t size;
	test_caseStart("Allocate, then free in \"random\" direction 2");
	checkMemoryBefore(false);
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		if(ptrs[randFree2[size]] != NULL) {
			tprintf("FREE4: address=0x%x, i=%d\n",ptrs[randFree2[size]],size);
			kheap_free(ptrs[randFree2[size]]);
		}
	}
	checkMemoryAfter(false);
}

/* allocate area 1, free area 1, ... */
static void test_kheap_t2(void) {
	size_t size;
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		test_caseStart("Allocate and free %d bytes",sizes[size] * sizeof(uint));
		checkMemoryBefore(false);

		ptrs[0] = (uint*)kheap_alloc(sizes[size] * sizeof(uint));
		if(ptrs[0] != NULL) {
			tprintf("Write test for 0x%x...",ptrs[0]);
			/* write test */
			*(ptrs[0]) = 1;
			*(ptrs[0] + sizes[size] - 1) = 2;
			tprintf("done\n");
			tprintf("Freeing mem @ 0x%x\n",ptrs[0]);
			kheap_free(ptrs[0]);

			checkMemoryAfter(false);
		}
		else {
			tprintf("Not enough mem\n");
			test_caseSucceeded();
		}
	}
}

/* allocate single bytes to reach the next page for the mem-area-structs */
static void test_kheap_t3(void) {
	size_t i;
	test_caseStart("Allocate %d times 1 byte",SINGLE_BYTE_COUNT);
	checkMemoryBefore(false);
	for(i = 0; i < SINGLE_BYTE_COUNT; i++) {
		ptrsSingle[i] = kheap_alloc(1);
	}
	for(i = 0; i < SINGLE_BYTE_COUNT; i++) {
		kheap_free(ptrsSingle[i]);
	}
	checkMemoryAfter(false);
}

/* reallocate test */
static void test_kheap_t5(void) {
	size_t i;
	uint *ptr1,*ptr2,*ptr3,*ptr4,*ptr5;
	test_caseStart("Allocating 3 regions");
	checkMemoryBefore(false);
	ptr1 = kheap_alloc(4 * sizeof(uint));
	for(i = 0;i < 4;i++)
		*(ptr1+i) = 1;
	ptr2 = kheap_alloc(8 * sizeof(uint));
	for(i = 0;i < 8;i++)
		*(ptr2+i) = 2;
	ptr3 = kheap_alloc(12 * sizeof(uint));
	for(i = 0;i < 12;i++)
		*(ptr3+i) = 3;
	tprintf("Freeing region 2...\n");
	kheap_free(ptr2);
	tprintf("Reusing region 2...\n");
	ptr4 = kheap_alloc(6 * sizeof(uint));
	for(i = 0;i < 6;i++)
		*(ptr4+i) = 4;
	ptr5 = kheap_alloc(2 * sizeof(uint));
	for(i = 0;i < 2;i++)
		*(ptr5+i) = 5;

	tprintf("Testing contents...\n");
	if(test_checkContent(ptr1,4,1) &&
			test_checkContent(ptr3,12,3) &&
			test_checkContent(ptr4,6,4) &&
			test_checkContent(ptr5,2,5))
		test_caseSucceeded();
	else
		test_caseFailed("Contents not equal");

	kheap_free(ptr1);
	kheap_free(ptr3);
	kheap_free(ptr4);
	kheap_free(ptr5);
	checkMemoryAfter(false);
}

static void test_kheap_realloc(void) {
	size_t i;
	uint *p,*ptr1,*ptr2,*ptr3;
	test_caseStart("Testing realloc()");
	checkMemoryBefore(false);

	ptr1 = (uint*)kheap_alloc(10 * sizeof(uint));
	for(p = ptr1,i = 0;i < 10;i++)
		*p++ = 1;

	ptr2 = (uint*)kheap_alloc(5 * sizeof(uint));
	for(p = ptr2,i = 0;i < 5;i++)
		*p++ = 2;

	ptr3 = (uint*)kheap_alloc(2 * sizeof(uint));
	for(p = ptr3,i = 0;i < 2;i++)
		*p++ = 3;

	ptr2 = (uint*)kheap_realloc(ptr2,10 * sizeof(uint));

	/* check content */
	if(!test_checkContent(ptr1,10,1)) {
		test_caseFailed("Content of unaffected area (ptr1) changed");
		return;
	}
	if(!test_checkContent(ptr3,2,3)) {
		test_caseFailed("Content of unaffected area (ptr3) changed");
		return;
	}
	if(!test_checkContent(ptr2,5,2)) {
		test_caseFailed("Content of affected area (ptr2) not copied");
		return;
	}

	/* fill 2 completely */
	for(p = ptr2,i = 0;i < 10;i++)
		*p++ = 2;

	ptr3 = (uint*)kheap_realloc(ptr3,6 * sizeof(uint));

	/* check content */
	if(!test_checkContent(ptr1,10,1)) {
		test_caseFailed("Content of unaffected area (ptr1) changed");
		return;
	}
	if(!test_checkContent(ptr2,10,2)) {
		test_caseFailed("Content of unaffected area (ptr2) changed");
		return;
	}
	if(!test_checkContent(ptr3,2,3)) {
		test_caseFailed("Content of affected area (ptr3) not copied");
		return;
	}

	/* fill 3 completely */
	for(p = ptr3,i = 0;i < 6;i++)
		*p++ = 3;

	ptr3 = (uint*)kheap_realloc(ptr3,7 * sizeof(uint));

	/* check content */
	if(!test_checkContent(ptr1,10,1)) {
		test_caseFailed("Content of unaffected area (ptr1) changed");
		return;
	}
	if(!test_checkContent(ptr2,10,2)) {
		test_caseFailed("Content of unaffected area (ptr2) changed");
		return;
	}
	if(!test_checkContent(ptr3,6,3)) {
		test_caseFailed("Content of affected area (ptr3) not copied");
		return;
	}

	/* fill 3 completely */
	for(p = ptr3,i = 0;i < 7;i++)
		*p++ = 3;

	/* free all */
	kheap_free(ptr1);
	kheap_free(ptr2);
	kheap_free(ptr3);

	checkMemoryAfter(false);
}
