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
#include <sys/mem/pagedir.h>
#include <sys/mem/cache.h>
#include <esc/test.h>

#include "tswapmap.h"

static void test_swapmap();
static void test_swapmap1();
static void test_swapmap2();
static void test_swapmap5();
static void test_swapmap6();
static void test_doStart(const char *title);
static void test_finish();

/* our test-module */
sTestModule tModSwapMap = {
	"SwapMap",
	&test_swapmap
};

static size_t spaceBefore;

static void test_swapmap() {
	test_swapmap1();
	test_swapmap2();
	test_swapmap5();
	test_swapmap6();
}

static void test_swapmap1() {
	ulong blocks[5];
	test_doStart("Testing alloc & free");

	blocks[0] = SwapMap::alloc();
	blocks[1] = SwapMap::alloc();
	blocks[2] = SwapMap::alloc();
	blocks[3] = SwapMap::alloc();
	blocks[4] = SwapMap::alloc();

	test_assertTrue(SwapMap::isUsed(blocks[0]));
	test_assertTrue(SwapMap::isUsed(blocks[1]));
	test_assertTrue(SwapMap::isUsed(blocks[2]));
	test_assertTrue(SwapMap::isUsed(blocks[3]));
	test_assertTrue(SwapMap::isUsed(blocks[4]));
	test_assertFalse(SwapMap::isUsed(blocks[4] - 1));
	test_assertFalse(SwapMap::isUsed(blocks[4] - 2));

	SwapMap::free(blocks[0]);
	SwapMap::free(blocks[1]);
	SwapMap::free(blocks[2]);
	SwapMap::free(blocks[3]);
	SwapMap::free(blocks[4]);

	test_finish();
}

static void test_swapmap2() {
	ulong blocks[5];
	test_doStart("Testing alloc & reverse free");

	blocks[0] = SwapMap::alloc();
	blocks[1] = SwapMap::alloc();
	blocks[2] = SwapMap::alloc();
	blocks[3] = SwapMap::alloc();
	blocks[4] = SwapMap::alloc();

	SwapMap::free(blocks[4]);
	SwapMap::free(blocks[3]);
	SwapMap::free(blocks[2]);
	SwapMap::free(blocks[1]);
	SwapMap::free(blocks[0]);

	test_finish();
}

static void test_swapmap5() {
	ulong blocks[9];
	test_doStart("Testing alloc & free mixed");

	blocks[0] = SwapMap::alloc();
	blocks[1] = SwapMap::alloc();
	blocks[2] = SwapMap::alloc();
	blocks[3] = SwapMap::alloc();

	test_assertTrue(SwapMap::isUsed(blocks[0]));
	test_assertTrue(SwapMap::isUsed(blocks[1]));
	test_assertTrue(SwapMap::isUsed(blocks[2]));
	test_assertTrue(SwapMap::isUsed(blocks[3]));

	SwapMap::free(blocks[2]);

	test_assertTrue(SwapMap::isUsed(blocks[0]));
	test_assertTrue(SwapMap::isUsed(blocks[1]));
	test_assertFalse(SwapMap::isUsed(blocks[2]));
	test_assertTrue(SwapMap::isUsed(blocks[3]));

	blocks[4] = SwapMap::alloc();
	blocks[5] = SwapMap::alloc();
	blocks[6] = SwapMap::alloc();
	blocks[7] = SwapMap::alloc();

	test_assertTrue(SwapMap::isUsed(blocks[4]));
	test_assertTrue(SwapMap::isUsed(blocks[5]));
	test_assertTrue(SwapMap::isUsed(blocks[6]));
	test_assertTrue(SwapMap::isUsed(blocks[7]));

	SwapMap::free(blocks[6]);

	test_assertTrue(SwapMap::isUsed(blocks[4]));
	test_assertTrue(SwapMap::isUsed(blocks[5]));
	test_assertFalse(SwapMap::isUsed(blocks[6]));
	test_assertTrue(SwapMap::isUsed(blocks[7]));

	blocks[8] = SwapMap::alloc();

	test_assertTrue(SwapMap::isUsed(blocks[8]));

	SwapMap::free(blocks[5]);
	SwapMap::free(blocks[3]);
	SwapMap::free(blocks[8]);
	SwapMap::free(blocks[4]);
	SwapMap::free(blocks[0]);
	SwapMap::free(blocks[1]);
	SwapMap::free(blocks[7]);

	test_finish();
}

static void test_swapmap6() {
	size_t total = SwapMap::freeSpace() / PAGE_SIZE;
	ulong *blocks = (ulong*)Cache::alloc(total * sizeof(ulong));
	test_doStart("Testing alloc all & free");

	for(size_t i = 0; i < total; i++) {
		blocks[i] = SwapMap::alloc();
		test_assertTrue(blocks[i] != INVALID_BLOCK);
	}
	for(size_t i = 0; i < total; i++)
		SwapMap::free(blocks[i]);

	test_finish();
	Cache::free(blocks);
}

static void test_doStart(const char *title) {
	test_caseStart(title);
	spaceBefore = SwapMap::freeSpace();
}

static void test_finish() {
	size_t spaceAfter = SwapMap::freeSpace();
	if(spaceAfter != spaceBefore)
		test_caseFailed("Space before: %d, After: %d",spaceBefore,spaceAfter);
	else
		test_caseSucceeded();
}
