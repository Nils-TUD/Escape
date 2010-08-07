/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/sllist.h>
#include <esc/test.h>

#include "tswapmap.h"

static void test_swapmap(void);
static void test_swapmap1(void);
static void test_swapmap2(void);
static void test_swapmap3(void);
static void test_swapmap4(void);
static void test_swapmap5(void);
static void test_swapmap6(void);
static void test_doStart(const char *title);
static void test_finish(void);

/* our test-module */
sTestModule tModSwapMap = {
	"SwapMap",
	&test_swapmap
};

static u32 spaceBefore;

static void test_swapmap(void) {
	test_swapmap1();
	test_swapmap2();
	test_swapmap3();
	test_swapmap4();
	test_swapmap5();
	test_swapmap6();
}

static void test_swapmap1(void) {
	u32 blocks[5];
	test_doStart("Testing alloc & free");

	blocks[0] = swmap_alloc(1);
	blocks[1] = swmap_alloc(3);
	blocks[2] = swmap_alloc(1);
	blocks[3] = swmap_alloc(1);
	blocks[4] = swmap_alloc(1);

	test_assertTrue(swmap_isUsed(blocks[0]));
	test_assertTrue(swmap_isUsed(blocks[1]));
	test_assertTrue(swmap_isUsed(blocks[1] + 1));
	test_assertTrue(swmap_isUsed(blocks[1] + 2));
	test_assertTrue(swmap_isUsed(blocks[3]));
	test_assertTrue(swmap_isUsed(blocks[4]));
	test_assertFalse(swmap_isUsed(blocks[4] + 1));
	test_assertFalse(swmap_isUsed(blocks[4] + 2));

	swmap_free(blocks[0],1);
	swmap_free(blocks[1] + 1,1);
	swmap_free(blocks[1] + 2,1);
	swmap_free(blocks[1],1);
	swmap_free(blocks[2],1);
	swmap_free(blocks[3],1);
	swmap_free(blocks[4],1);

	test_finish();
}

static void test_swapmap2(void) {
	u32 blocks[5];
	test_doStart("Testing alloc & reverse free");

	blocks[0] = swmap_alloc(1);
	blocks[1] = swmap_alloc(2);
	blocks[2] = swmap_alloc(1);
	blocks[3] = swmap_alloc(1);
	blocks[4] = swmap_alloc(1);

	swmap_free(blocks[4],1);
	swmap_free(blocks[3],1);
	swmap_free(blocks[2],1);
	swmap_free(blocks[1],2);
	swmap_free(blocks[0],1);

	test_finish();
}

static void test_swapmap3(void) {
	test_doStart("Testing alloc & area free");

	swmap_alloc(1);
	swmap_alloc(2);
	swmap_alloc(1);
	swmap_alloc(1);
	swmap_alloc(1);

	swmap_free(0,3);
	swmap_free(3,1);
	swmap_free(5,1);
	swmap_free(4,1);

	test_finish();
}

static void test_swapmap4(void) {
	u32 blocks[8];
	test_doStart("Testing alloc & random free");

	blocks[0] = swmap_alloc(1);
	blocks[1] = swmap_alloc(2);
	blocks[2] = swmap_alloc(4);
	blocks[3] = swmap_alloc(1);
	blocks[4] = swmap_alloc(1);
	blocks[5] = swmap_alloc(3);
	blocks[6] = swmap_alloc(1);
	blocks[7] = swmap_alloc(1);

	test_assertTrue(swmap_isUsed(blocks[0]));
	test_assertTrue(swmap_isUsed(blocks[1]));
	test_assertTrue(swmap_isUsed(blocks[1] + 1));
	test_assertTrue(swmap_isUsed(blocks[2]));
	test_assertTrue(swmap_isUsed(blocks[2] + 1));
	test_assertTrue(swmap_isUsed(blocks[2] + 2));
	test_assertTrue(swmap_isUsed(blocks[2] + 3));
	test_assertTrue(swmap_isUsed(blocks[3]));
	test_assertTrue(swmap_isUsed(blocks[4]));
	test_assertTrue(swmap_isUsed(blocks[5]));
	test_assertTrue(swmap_isUsed(blocks[5] + 1));
	test_assertTrue(swmap_isUsed(blocks[5] + 2));
	test_assertTrue(swmap_isUsed(blocks[6]));
	test_assertTrue(swmap_isUsed(blocks[7]));

	swmap_free(blocks[5],2);
	swmap_free(blocks[5] + 2,1);
	swmap_free(blocks[7],1);
	swmap_free(blocks[0],1);
	swmap_free(blocks[6],1);
	swmap_free(blocks[4],1);
	swmap_free(blocks[1],1);
	swmap_free(blocks[1] + 1,1);
	swmap_free(blocks[2],4);
	swmap_free(blocks[3],1);

	test_finish();
}

static void test_swapmap5(void) {
	u32 blocks[8];
	test_doStart("Testing alloc & free mixed");

	blocks[0] = swmap_alloc(2);
	blocks[1] = swmap_alloc(1);
	blocks[2] = swmap_alloc(1);
	blocks[3] = swmap_alloc(1);

	test_assertTrue(swmap_isUsed(blocks[0]));
	test_assertTrue(swmap_isUsed(blocks[0] + 1));
	test_assertTrue(swmap_isUsed(blocks[1]));
	test_assertTrue(swmap_isUsed(blocks[2]));
	test_assertTrue(swmap_isUsed(blocks[3]));

	swmap_free(blocks[2],1);

	test_assertTrue(swmap_isUsed(blocks[0]));
	test_assertTrue(swmap_isUsed(blocks[0] + 1));
	test_assertTrue(swmap_isUsed(blocks[1]));
	test_assertFalse(swmap_isUsed(blocks[2]));
	test_assertTrue(swmap_isUsed(blocks[3]));

	blocks[4] = swmap_alloc(1);
	blocks[5] = swmap_alloc(1);
	blocks[6] = swmap_alloc(1);
	blocks[7] = swmap_alloc(1);

	test_assertTrue(swmap_isUsed(blocks[4]));
	test_assertTrue(swmap_isUsed(blocks[5]));
	test_assertTrue(swmap_isUsed(blocks[6]));
	test_assertTrue(swmap_isUsed(blocks[7]));

	swmap_free(blocks[6],1);

	test_assertTrue(swmap_isUsed(blocks[4]));
	test_assertTrue(swmap_isUsed(blocks[5]));
	test_assertFalse(swmap_isUsed(blocks[6]));
	test_assertTrue(swmap_isUsed(blocks[7]));

	blocks[8] = swmap_alloc(1);

	test_assertTrue(swmap_isUsed(blocks[8]));

	swmap_free(blocks[5],1);
	swmap_free(blocks[8],2);
	swmap_free(blocks[0],5);

	test_finish();
}

static void test_swapmap6(void) {
	u32 addr;
	test_doStart("Testing alloc all & free");

	addr = 0;
	while(swmap_freeSpace() > 1 * PAGE_SIZE) {
		test_assertTrue(swmap_alloc(1) != INVALID_BLOCK);
		addr += PAGE_SIZE;
	}
	swmap_free(0,addr / PAGE_SIZE);

	test_finish();
}

static void test_doStart(const char *title) {
	test_caseStart(title);
	spaceBefore = swmap_freeSpace();
}

static void test_finish(void) {
	u32 spaceAfter = swmap_freeSpace();
	if(spaceAfter != spaceBefore)
		test_caseFailed("Space before: %d, After: %d",spaceBefore,spaceAfter);
	else
		test_caseSucceded();
}
