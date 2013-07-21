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
#include <sys/task/thread.h>
#include <sys/mem/vmfree.h>
#include <sys/util.h>
#include <esc/test.h>
#include "testutils.h"

#define TOTAL_SIZE	(0x40 * PAGE_SIZE)
#define AREA_COUNT	10

static void test_vmfree(void);
static void test_vmfree_inOrder(void);
static void test_vmfree_revOrder(void);
static void test_vmfree_randOrder(void);
static void test_vmfree_allocAt(void);
static void test_vmfree_allocNFree(size_t *sizes,size_t *freeIndices,const char *msg);

/* our test-module */
sTestModule tModVMFree = {
	"VM-FreeArea",
	&test_vmfree
};

static void test_vmfree(void) {
	test_vmfree_inOrder();
	test_vmfree_revOrder();
	test_vmfree_randOrder();
	test_vmfree_allocAt();
}

static void test_vmfree_inOrder(void) {
	size_t i,sizes[AREA_COUNT];
	size_t freeIndices[AREA_COUNT];
	for(i = 0; i < AREA_COUNT; i++) {
		sizes[i] = (i + 1) * PAGE_SIZE;
		freeIndices[i] = i;
	}
	test_vmfree_allocNFree(sizes,freeIndices,"Allocating areas and freeing them in same order");
}

static void test_vmfree_revOrder(void) {
	size_t i,sizes[AREA_COUNT];
	size_t freeIndices[AREA_COUNT];
	for(i = 0; i < AREA_COUNT; i++) {
		sizes[i] = (i + 1) * PAGE_SIZE;
		freeIndices[i] = AREA_COUNT - i - 1;
	}
	test_vmfree_allocNFree(sizes,freeIndices,"Allocating areas and freeing them in reverse order");
}

static void test_vmfree_randOrder(void) {
	size_t i,sizes[AREA_COUNT];
	size_t freeIndices[AREA_COUNT];
	for(i = 0; i < AREA_COUNT; i++) {
		sizes[i] = (i + 1) * PAGE_SIZE;
		freeIndices[i] = i;
	}
	util_srand(0x12345);
	for(i = 0; i < 10000; i++) {
		size_t j = util_rand() % AREA_COUNT;
		size_t k = util_rand() % AREA_COUNT;
		size_t t = freeIndices[j];
		freeIndices[j] = freeIndices[k];
		freeIndices[k] = t;
	}
	test_vmfree_allocNFree(sizes,freeIndices,"Allocating areas and freeing them in random order");
}

static void test_vmfree_allocAt(void) {
	VMFreeMap map;
	size_t areas,size = TOTAL_SIZE;
	test_caseStart("Allocating areas at specific positions");
	checkMemoryBefore(false);

	test_assertTrue(VMFreeMap::init(&map,PAGE_SIZE,size));
	test_assertSize(map.getSize(&areas),size);
	test_assertSize(areas,1);

	test_assertTrue(map.allocateAt(1 * PAGE_SIZE,2 * PAGE_SIZE));
	test_assertTrue(map.allocateAt(3 * PAGE_SIZE,1 * PAGE_SIZE));
	test_assertTrue(map.allocateAt(6 * PAGE_SIZE,2 * PAGE_SIZE));
	test_assertTrue(map.allocateAt(5 * PAGE_SIZE,1 * PAGE_SIZE));
	test_assertTrue(map.allocateAt(18 * PAGE_SIZE,1 * PAGE_SIZE));
	test_assertTrue(map.allocateAt(16 * PAGE_SIZE,1 * PAGE_SIZE));
	test_assertTrue(map.allocateAt(4 * PAGE_SIZE,1 * PAGE_SIZE));

	map.free(18 * PAGE_SIZE,1 * PAGE_SIZE);
	map.free(6 * PAGE_SIZE,2 * PAGE_SIZE);
	map.free(5 * PAGE_SIZE,1 * PAGE_SIZE);
	map.free(1 * PAGE_SIZE,2 * PAGE_SIZE);
	map.free(4 * PAGE_SIZE,1 * PAGE_SIZE);
	map.free(3 * PAGE_SIZE,1 * PAGE_SIZE);
	map.free(16 * PAGE_SIZE,1 * PAGE_SIZE);

	test_assertSize(map.getSize(&areas),size);
	test_assertSize(areas,1);
	map.destroy();

	checkMemoryAfter(false);
	test_caseSucceeded();
}

static void test_vmfree_allocNFree(size_t *sizes,size_t *freeIndices,const char *msg) {
	size_t i,areas;
	uintptr_t addrs[AREA_COUNT];
	size_t size = TOTAL_SIZE;
	VMFreeMap map;
	test_caseStart(msg);
	checkMemoryBefore(false);

	test_assertTrue(VMFreeMap::init(&map,PAGE_SIZE,size));
	test_assertSize(map.getSize(&areas),size);
	test_assertSize(areas,1);

	/* allocate */
	for(i = 0; i < AREA_COUNT; i++) {
		addrs[i] = map.allocate(sizes[i]);
		test_assertTrue(addrs[i] > 0);
		size -= sizes[i];
		test_assertSize(map.getSize(&areas),size);
	}

	/* free */
	for(i = 0; i < AREA_COUNT; i++) {
		map.free(addrs[freeIndices[i]],sizes[freeIndices[i]]);
		size += sizes[freeIndices[i]];
		test_assertSize(map.getSize(&areas),size);
	}

	test_assertSize(map.getSize(&areas),size);
	test_assertSize(areas,1);
	map.destroy();

	checkMemoryAfter(false);
	test_caseSucceeded();
}
