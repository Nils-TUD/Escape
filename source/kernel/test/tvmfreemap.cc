/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <mem/vmfreemap.h>
#include <sys/test.h>
#include <task/thread.h>
#include <common.h>
#include <util.h>

#include "testutils.h"

#define TOTAL_SIZE	(0x40 * PAGE_SIZE)
#define AREA_COUNT	10

static void test_vmfree();
static void test_vmfree_inOrder();
static void test_vmfree_revOrder();
static void test_vmfree_randOrder();
static void test_vmfree_allocAt();
static void test_vmfree_allocNFree(size_t *sizes,size_t *freeIndices,const char *msg);

/* our test-module */
sTestModule tModVMFree = {
	"VM-FreeArea",
	&test_vmfree
};

static void test_vmfree() {
	test_vmfree_inOrder();
	test_vmfree_revOrder();
	test_vmfree_randOrder();
	test_vmfree_allocAt();
}

static void test_vmfree_inOrder() {
	size_t sizes[AREA_COUNT];
	size_t freeIndices[AREA_COUNT];
	for(size_t i = 0; i < AREA_COUNT; i++) {
		sizes[i] = (i + 1) * PAGE_SIZE;
		freeIndices[i] = i;
	}
	test_vmfree_allocNFree(sizes,freeIndices,"Allocating areas and freeing them in same order");
}

static void test_vmfree_revOrder() {
	size_t sizes[AREA_COUNT];
	size_t freeIndices[AREA_COUNT];
	for(size_t i = 0; i < AREA_COUNT; i++) {
		sizes[i] = (i + 1) * PAGE_SIZE;
		freeIndices[i] = AREA_COUNT - i - 1;
	}
	test_vmfree_allocNFree(sizes,freeIndices,"Allocating areas and freeing them in reverse order");
}

static void test_vmfree_randOrder() {
	size_t sizes[AREA_COUNT];
	size_t freeIndices[AREA_COUNT];
	for(size_t i = 0; i < AREA_COUNT; i++) {
		sizes[i] = (i + 1) * PAGE_SIZE;
		freeIndices[i] = i;
	}
	Util::srand(0x12345);
	for(size_t i = 0; i < 10000; i++) {
		size_t j = Util::rand() % AREA_COUNT;
		size_t k = Util::rand() % AREA_COUNT;
		size_t t = freeIndices[j];
		freeIndices[j] = freeIndices[k];
		freeIndices[k] = t;
	}
	test_vmfree_allocNFree(sizes,freeIndices,"Allocating areas and freeing them in random order");
}

static void test_vmfree_allocAt() {
	size_t areas,size = TOTAL_SIZE;
	test_caseStart("Allocating areas at specific positions");
	checkMemoryBefore(false);

	{
		VMFreeMap map(PAGE_SIZE,size);

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
	}

	checkMemoryAfter(false);
	test_caseSucceeded();
}

static void test_vmfree_allocNFree(size_t *sizes,size_t *freeIndices,const char *msg) {
	size_t areas;
	uintptr_t addrs[AREA_COUNT];
	size_t size = TOTAL_SIZE;
	test_caseStart(msg);
	checkMemoryBefore(false);

	{
		VMFreeMap map(PAGE_SIZE,size);

		test_assertSize(map.getSize(&areas),size);
		test_assertSize(areas,1);

		/* allocate */
		for(size_t i = 0; i < AREA_COUNT; i++) {
			addrs[i] = map.allocate(sizes[i]);
			test_assertTrue(addrs[i] > 0);
			size -= sizes[i];
			test_assertSize(map.getSize(&areas),size);
		}

		/* free */
		for(size_t i = 0; i < AREA_COUNT; i++) {
			map.free(addrs[freeIndices[i]],sizes[freeIndices[i]]);
			size += sizes[freeIndices[i]];
			test_assertSize(map.getSize(&areas),size);
		}

		test_assertSize(map.getSize(&areas),size);
		test_assertSize(areas,1);
	}

	checkMemoryAfter(false);
	test_caseSucceeded();
}
