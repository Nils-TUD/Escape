/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/kheap.h"
#include "../h/paging.h"
#include "../h/mm.h"
#include "../h/video.h"
#include "tkheap.h"
#include <stdarg.h>

/* forward declarations */
static void test_kheap(void);
static void test_kheap_t1v1(void);
static void test_kheap_t1v2(void);
static void test_kheap_t1v3(void);
static void test_kheap_t1v4(void);
static void test_kheap_t2(void);
static void test_kheap_t3(void);
static void test_kheap_t4(void);
static void test_kheap_t5(void);

/* our test-module */
tTestModule tModKHeap = {
	"Kernel-Heap",
	&test_kheap
};

#define SINGLE_BYTE_COUNT 50000
u32 *ptrsSingle[SINGLE_BYTE_COUNT];

u32 sizes[] = {1,4,10,1023,1024,1025,2048,4097};
u32 *ptrs[ARRAY_SIZE(sizes)];
u32 randFree1[] = {7,5,2,0,6,3,4,1};
u32 randFree2[] = {3,4,1,5,6,0,7,2};

u32 oldPC, newPC;
u32 oldFC, newFC;
u32 oldFH, newFH;

/* helper */

static void test_init(cstring fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	test_caseStartv(fmt,ap);
	va_end(ap);

	oldPC = paging_getPageCount();
	oldFC = mm_getNumberOfFreeFrames(MM_DEF);
	oldFH = kheap_getFreeMem();
}

static void test_check(void) {
	newPC = paging_getPageCount();
	newFC = mm_getNumberOfFreeFrames(MM_DEF);
	newFH = kheap_getFreeMem();
	if(newPC != oldPC || newFC != oldFC || newFH != oldFH) {
		test_caseFailed("old-page-count=%d, new-page-count=%d,"
				" old-frame-count=%d, new-frame-count=%d, old-heap-count=%d, new-heap-count=%d",
				oldPC,newPC,oldFC,newFC,oldFH,newFH);
	}
	else {
		test_caseSucceded();
	}
}

static bool test_checkContent(u32 *ptr,u32 count,u32 value) {
	while(count-- > 0) {
		if(*ptr++ != value)
			return false;
	}
	return true;
}

static void test_t1alloc(void) {
	u32 size;
	tprintf("Allocating...(%d free frames)\n",mm_getNumberOfFreeFrames(MM_DEF));
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		tprintf("%d bytes\n",sizes[size] * sizeof(u32));
		ptrs[size] = (u32*)kheap_alloc(sizes[size] * sizeof(u32));
		tprintf("Write test for 0x%x...",ptrs[size]);
		/* write test */
		*(ptrs[size]) = 4;
		*(ptrs[size] + sizes[size] - 1) = 2;
		tprintf("done\n");
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
		&test_kheap_t4,
		&test_kheap_t5
	};

	u32 i;
	for(i = 0; i < ARRAY_SIZE(tests); i++) {
		tests[i]();
	}
}

/* test functions */

/* allocate, free in same direction */
static void test_kheap_t1v1(void) {
	u32 size;
	test_init("Allocate, then free in same direction");
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		tprintf("FREE1: address=0x%x, i=%d\n",ptrs[size],size);
		kheap_free(ptrs[size]);
		/* write test */
		u32 i;
		for(i = size + 1; i < ARRAY_SIZE(sizes); i++) {
			*(ptrs[i]) = 1;
			*(ptrs[i] + sizes[i] - 1) = 2;
		}
	}
	test_check();
}

/* allocate, free in opposite direction */
static void test_kheap_t1v2(void) {
	s32 size;
	test_init("Allocate, then free in opposite direction");
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = ARRAY_SIZE(sizes) - 1; size >= 0; size--) {
		tprintf("FREE2: address=0x%x, i=%d\n",ptrs[size],size);
		kheap_free(ptrs[size]);
		/* write test */
		s32 i;
		for(i = size - 1; i >= 0; i--) {
			*(ptrs[i]) = 1;
			*(ptrs[i] + sizes[i] - 1) = 2;
		}
	}
	return test_check();
}

/* allocate, free in random direction 1 */
static void test_kheap_t1v3(void) {
	u32 size;
	test_init("Allocate, then free in \"random\" direction 1");
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = 0; (u32)size < ARRAY_SIZE(sizes); size++) {
		tprintf("FREE3: address=0x%x, i=%d\n",ptrs[randFree1[size]],size);
		kheap_free(ptrs[randFree1[size]]);
	}
	test_check();
}

/* allocate, free in random direction 2 */
static void test_kheap_t1v4(void) {
	u32 size;
	test_init("Allocate, then free in \"random\" direction 2");
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = 0; (u32)size < ARRAY_SIZE(sizes); size++) {
		tprintf("FREE4: address=0x%x, i=%d\n",ptrs[randFree2[size]],size);
		kheap_free(ptrs[randFree2[size]]);
	}
	test_check();
}

/* allocate area 1, free area 1, ... */
static void test_kheap_t2(void) {
	u32 size;
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		test_init("Allocate and free %d bytes",sizes[size] * sizeof(u32));

		ptrs[0] = (u32*)kheap_alloc(sizes[size] * sizeof(u32));
		tprintf("Write test for 0x%x...",ptrs[0]);
		/* write test */
		*(ptrs[0]) = 1;
		*(ptrs[0] + sizes[size] - 1) = 2;
		tprintf("done\n");
		tprintf("Freeing mem @ 0x%x\n",ptrs[0]);
		kheap_free(ptrs[0]);

		test_check();
	}
}

/* allocate single bytes to reach the next page for the mem-area-structs */
static void test_kheap_t3(void) {
	test_init("Allocate %d times 1 byte",SINGLE_BYTE_COUNT);
	u32 i;
	for(i = 0; i < SINGLE_BYTE_COUNT; i++) {
		ptrsSingle[i] = kheap_alloc(1);
	}
	for(i = 0; i < SINGLE_BYTE_COUNT; i++) {
		kheap_free(ptrsSingle[i]);
	}
	test_check();
}

/* allocate all */
static void test_kheap_t4(void) {
	test_init("Allocate all and free it");
	u32 *ptr = kheap_alloc(kheap_getFreeMem() - 1);
	kheap_free(ptr);
	test_check();
}

/* reallocate test */
static void test_kheap_t5(void) {
	u32 i;
	u32 *ptr1,*ptr2,*ptr3,*ptr4,*ptr5;
	test_init("Allocating 3 regions");
	ptr1 = kheap_alloc(4 * sizeof(u32));
	for(i = 0;i < 4;i++)
		*(ptr1+i) = 1;
	ptr2 = kheap_alloc(8 * sizeof(u32));
	for(i = 0;i < 8;i++)
		*(ptr2+i) = 2;
	ptr3 = kheap_alloc(12 * sizeof(u32));
	for(i = 0;i < 12;i++)
		*(ptr3+i) = 3;
	tprintf("Freeing region 2...\n");
	kheap_free(ptr2);
	tprintf("Reusing region 2...\n");
	ptr4 = kheap_alloc(6 * sizeof(u32));
	for(i = 0;i < 6;i++)
		*(ptr4+i) = 4;
	ptr5 = kheap_alloc(2 * sizeof(u32));
	for(i = 0;i < 2;i++)
		*(ptr5+i) = 5;

	tprintf("Testing contents...\n");
	if(test_checkContent(ptr1,4,1) &&
			test_checkContent(ptr3,12,3) &&
			test_checkContent(ptr4,6,4) &&
			test_checkContent(ptr5,2,5))
		test_caseSucceded();
	else
		test_caseFailed("Contents not equal");

	kheap_free(ptr1);
	kheap_free(ptr3);
	kheap_free(ptr4);
	kheap_free(ptr5);
}
