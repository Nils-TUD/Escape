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
#include <sys/mem/swapmap.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <esc/sllist.h>
#include <esc/test.h>

#include "tswapmap.h"

static void test_swapmap(void);
static void test_swapmap1(void);
static void test_swapmap2(void);
static void test_swapmap5(void);
static void test_swapmap6(void);
static void test_doStart(const char *title);
static void test_finish(void);

/* our test-module */
sTestModule tModSwapMap = {
	"SwapMap",
	&test_swapmap
};

static size_t spaceBefore;

static void test_swapmap(void) {
	test_swapmap1();
	test_swapmap2();
	test_swapmap5();
	test_swapmap6();
}

static void test_swapmap1(void) {
	ulong blocks[5];
	test_doStart("Testing alloc & free");

	blocks[0] = swmap_alloc();
	blocks[1] = swmap_alloc();
	blocks[2] = swmap_alloc();
	blocks[3] = swmap_alloc();
	blocks[4] = swmap_alloc();

	test_assertTrue(swmap_isUsed(blocks[0]));
	test_assertTrue(swmap_isUsed(blocks[1]));
	test_assertTrue(swmap_isUsed(blocks[2]));
	test_assertTrue(swmap_isUsed(blocks[3]));
	test_assertTrue(swmap_isUsed(blocks[4]));
	test_assertFalse(swmap_isUsed(blocks[4] - 1));
	test_assertFalse(swmap_isUsed(blocks[4] - 2));

	swmap_free(blocks[0]);
	swmap_free(blocks[1]);
	swmap_free(blocks[2]);
	swmap_free(blocks[3]);
	swmap_free(blocks[4]);

	test_finish();
}

static void test_swapmap2(void) {
	ulong blocks[5];
	test_doStart("Testing alloc & reverse free");

	blocks[0] = swmap_alloc();
	blocks[1] = swmap_alloc();
	blocks[2] = swmap_alloc();
	blocks[3] = swmap_alloc();
	blocks[4] = swmap_alloc();

	swmap_free(blocks[4]);
	swmap_free(blocks[3]);
	swmap_free(blocks[2]);
	swmap_free(blocks[1]);
	swmap_free(blocks[0]);

	test_finish();
}

static void test_swapmap5(void) {
	ulong blocks[9];
	test_doStart("Testing alloc & free mixed");

	blocks[0] = swmap_alloc();
	blocks[1] = swmap_alloc();
	blocks[2] = swmap_alloc();
	blocks[3] = swmap_alloc();

	test_assertTrue(swmap_isUsed(blocks[0]));
	test_assertTrue(swmap_isUsed(blocks[1]));
	test_assertTrue(swmap_isUsed(blocks[2]));
	test_assertTrue(swmap_isUsed(blocks[3]));

	swmap_free(blocks[2]);

	test_assertTrue(swmap_isUsed(blocks[0]));
	test_assertTrue(swmap_isUsed(blocks[1]));
	test_assertFalse(swmap_isUsed(blocks[2]));
	test_assertTrue(swmap_isUsed(blocks[3]));

	blocks[4] = swmap_alloc();
	blocks[5] = swmap_alloc();
	blocks[6] = swmap_alloc();
	blocks[7] = swmap_alloc();

	test_assertTrue(swmap_isUsed(blocks[4]));
	test_assertTrue(swmap_isUsed(blocks[5]));
	test_assertTrue(swmap_isUsed(blocks[6]));
	test_assertTrue(swmap_isUsed(blocks[7]));

	swmap_free(blocks[6]);

	test_assertTrue(swmap_isUsed(blocks[4]));
	test_assertTrue(swmap_isUsed(blocks[5]));
	test_assertFalse(swmap_isUsed(blocks[6]));
	test_assertTrue(swmap_isUsed(blocks[7]));

	blocks[8] = swmap_alloc();

	test_assertTrue(swmap_isUsed(blocks[8]));

	swmap_free(blocks[5]);
	swmap_free(blocks[3]);
	swmap_free(blocks[8]);
	swmap_free(blocks[4]);
	swmap_free(blocks[0]);
	swmap_free(blocks[1]);
	swmap_free(blocks[7]);

	test_finish();
}

static void test_swapmap6(void) {
	size_t i;
	size_t total = swmap_freeSpace() / PAGE_SIZE;
	ulong *blocks = (ulong*)Cache::alloc(total * sizeof(ulong));
	test_doStart("Testing alloc all & free");

	for(i = 0; i < total; i++) {
		blocks[i] = swmap_alloc();
		test_assertTrue(blocks[i] != INVALID_BLOCK);
	}
	for(i = 0; i < total; i++)
		swmap_free(blocks[i]);

	test_finish();
	Cache::free(blocks);
}

static void test_doStart(const char *title) {
	test_caseStart(title);
	spaceBefore = swmap_freeSpace();
}

static void test_finish(void) {
	size_t spaceAfter = swmap_freeSpace();
	if(spaceAfter != spaceBefore)
		test_caseFailed("Space before: %d, After: %d",spaceBefore,spaceAfter);
	else
		test_caseSucceeded();
}
