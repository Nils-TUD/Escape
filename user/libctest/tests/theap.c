/**
 * @version		$Id: tkheap.c 99 2009-01-04 11:19:16Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/heap.h>
#include <esc/bufio.h>
#include <test.h>
#include <stdarg.h>
#include "theap.h"

/* forward declarations */
static void test_heap(void);
static void test_t1alloc(void);

static void test_heap_t1v1(void);
static void test_heap_t1v2(void);
static void test_heap_t1v3(void);
static void test_heap_t1v4(void);
static void test_heap_t2(void);
static void test_heap_t3(void);

/* our test-module */
sTestModule tModHeap = {
	"Heap",
	&test_heap
};

#define SINGLE_BYTE_COUNT 100000
u32 *ptrsSingle[SINGLE_BYTE_COUNT];

u32 sizes[] = {1,2,4,1023,1024,1025,2048,4097};
u32 *ptrs[ARRAY_SIZE(sizes)];
u32 randFree1[] = {7,5,2,0,6,3,4,1};
u32 randFree2[] = {3,4,1,5,6,0,7,2};

u32 oldFree, newFree;

static void test_heap(void) {
	void (*tests[])(void) = {
		&test_heap_t1v1,
		&test_heap_t1v2,
		&test_heap_t1v3,
		&test_heap_t1v4,
		&test_heap_t2,
		&test_heap_t3,
	};

	u32 i;
	for(i = 0; i < ARRAY_SIZE(tests); i++) {
		tests[i]();
	}
}

/* helper */

static void test_init(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	test_caseStartv(fmt,ap);
	va_end(ap);

	oldFree = heap_getFreeSpace();
}

static void test_check(void) {
	newFree = heap_getFreeSpace();
	if(newFree < oldFree) {
		test_caseFailed("Mem not free'd: oldFree=%u, newFree=%u",oldFree,newFree);
	}
	else {
		test_caseSucceded();
	}
}

static void test_t1alloc(void) {
	u32 size;
	tprintf("Allocating...\n");
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		tprintf("%d bytes\n",sizes[size] * sizeof(u32));
		ptrs[size] = (u32*)malloc(sizes[size] * sizeof(u32));
		tprintf("Write test for 0x%x...",ptrs[size]);
		/* write test */
		*(ptrs[size]) = 4;
		*(ptrs[size] + sizes[size] - 1) = 2;
		tprintf("done\n");
	}
}

/* test functions */

/* allocate, free in same direction */
static void test_heap_t1v1(void) {
	u32 size;
	test_init("Allocate, then free in same direction");
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		tprintf("FREE1: address=0x%x, i=%d\n",ptrs[size],size);
		free(ptrs[size]);
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
static void test_heap_t1v2(void) {
	s32 size;
	test_init("Allocate, then free in opposite direction");
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = ARRAY_SIZE(sizes) - 1; size >= 0; size--) {
		tprintf("FREE2: address=0x%x, i=%d\n",ptrs[size],size);
		free(ptrs[size]);
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
static void test_heap_t1v3(void) {
	u32 size;
	test_init("Allocate, then free in \"random\" direction 1");
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = 0; (u32)size < ARRAY_SIZE(sizes); size++) {
		tprintf("FREE3: address=0x%x, i=%d\n",ptrs[randFree1[size]],size);
		free(ptrs[randFree1[size]]);
	}
	test_check();
}

/* allocate, free in random direction 2 */
static void test_heap_t1v4(void) {
	u32 size;
	test_init("Allocate, then free in \"random\" direction 2");
	test_t1alloc();
	tprintf("Freeing...\n");
	for(size = 0; (u32)size < ARRAY_SIZE(sizes); size++) {
		tprintf("FREE4: address=0x%x, i=%d\n",ptrs[randFree2[size]],size);
		free(ptrs[randFree2[size]]);
	}
	test_check();
}

/* allocate area 1, free area 1, ... */
static void test_heap_t2(void) {
	u32 size;
	for(size = 0; size < ARRAY_SIZE(sizes); size++) {
		test_init("Allocate and free %d bytes",sizes[size] * sizeof(u32));

		ptrs[0] = (u32*)malloc(sizes[size] * sizeof(u32));
		tprintf("Write test for 0x%x...",ptrs[0]);
		/* write test */
		*(ptrs[0]) = 1;
		*(ptrs[0] + sizes[size] - 1) = 2;
		tprintf("done\n");
		tprintf("Freeing mem @ 0x%x\n",ptrs[0]);
		free(ptrs[0]);

		test_check();
	}
}

/* allocate single bytes */
static void test_heap_t3(void) {
	test_init("Allocate %d times 1 byte",SINGLE_BYTE_COUNT);
	u32 i;
	for(i = 0; i < SINGLE_BYTE_COUNT; i++) {
		ptrsSingle[i] = malloc(1);
	}
	for(i = 0; i < SINGLE_BYTE_COUNT; i++) {
		free(ptrsSingle[i]);
	}
	test_check();
}
