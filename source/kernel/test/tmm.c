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
#include <sys/mem/pmem.h>
#include <esc/test.h>
#include "tmm.h"

#define FRAME_COUNT 50

/* forward declarations */
static void test_mm(void);
static void test_default(void);
static void test_contiguous(void);
static void test_contiguous_align(void);
static void test_mm_allocate(void);
static void test_mm_free(void);

/* our test-module */
sTestModule tModMM = {
	"Physical memory-management",
	&test_mm
};

static tFrameNo frames[FRAME_COUNT];

static void test_mm(void) {
	test_default();
	test_contiguous();
	test_contiguous_align();
}

static void test_default(void) {
	size_t freeDefFrames;

	test_caseStart("Requesting and freeing %d frames",FRAME_COUNT);

	freeDefFrames = pmem_getFreeFrames(MM_DEF);
	test_mm_allocate();
	test_mm_free();
	test_assertUInt(pmem_getFreeFrames(MM_DEF),freeDefFrames);

	test_caseSucceded();
}

static void test_contiguous(void) {
	ssize_t res1,res2,res3,res4;
	size_t freeContFrames;

	test_caseStart("Requesting once and free");
	freeContFrames = pmem_getFreeFrames(MM_CONT);
	res1 = pmem_allocateContiguous(3,1);
	pmem_freeContiguous(res1,3);
	test_assertUInt(pmem_getFreeFrames(MM_CONT),freeContFrames);
	test_caseSucceded();

	test_caseStart("Requesting twice and free");
	freeContFrames = pmem_getFreeFrames(MM_CONT);
	res1 = pmem_allocateContiguous(6,1);
	res2 = pmem_allocateContiguous(5,1);
	pmem_freeContiguous(res1,6);
	pmem_freeContiguous(res2,5);
	test_assertUInt(pmem_getFreeFrames(MM_CONT),freeContFrames);
	test_caseSucceded();

	test_caseStart("Request, free, request and free");
	freeContFrames = pmem_getFreeFrames(MM_CONT);
	res1 = pmem_allocateContiguous(5,1);
	res2 = pmem_allocateContiguous(5,1);
	res3 = pmem_allocateContiguous(5,1);
	pmem_freeContiguous(res2,5);
	res2 = pmem_allocateContiguous(3,1);
	res4 = pmem_allocateContiguous(3,1);
	pmem_freeContiguous(res1,5);
	pmem_freeContiguous(res2,3);
	pmem_freeContiguous(res3,5);
	pmem_freeContiguous(res4,3);
	test_assertUInt(pmem_getFreeFrames(MM_CONT),freeContFrames);
	test_caseSucceded();

	test_caseStart("Request a lot multiple times and free");
	freeContFrames = pmem_getFreeFrames(MM_CONT);
	res1 = pmem_allocateContiguous(35,1);
	res2 = pmem_allocateContiguous(12,1);
	res3 = pmem_allocateContiguous(89,1);
	res4 = pmem_allocateContiguous(56,1);
	pmem_freeContiguous(res3,89);
	pmem_freeContiguous(res1,35);
	pmem_freeContiguous(res2,12);
	pmem_freeContiguous(res4,56);
	test_assertUInt(pmem_getFreeFrames(MM_CONT),freeContFrames);
	test_caseSucceded();
}

static void test_contiguous_align(void) {
	ssize_t res1,res2,res3,res4;
	size_t freeContFrames;

	test_caseStart("[Align] Requesting once and free");
	freeContFrames = pmem_getFreeFrames(MM_CONT);
	res1 = pmem_allocateContiguous(3,4);
	test_assertTrue((res1 % 4) == 0);
	pmem_freeContiguous(res1,3);
	test_assertUInt(pmem_getFreeFrames(MM_CONT),freeContFrames);
	test_caseSucceded();

	test_caseStart("[Align] Requesting twice and free");
	freeContFrames = pmem_getFreeFrames(MM_CONT);
	res1 = pmem_allocateContiguous(6,4);
	test_assertTrue((res1 % 4) == 0);
	res2 = pmem_allocateContiguous(5,8);
	test_assertTrue((res2 % 8) == 0);
	pmem_freeContiguous(res1,6);
	pmem_freeContiguous(res2,5);
	test_assertUInt(pmem_getFreeFrames(MM_CONT),freeContFrames);
	test_caseSucceded();

	test_caseStart("[Align] Request, free, request and free");
	freeContFrames = pmem_getFreeFrames(MM_CONT);
	res1 = pmem_allocateContiguous(5,16);
	test_assertTrue((res1 % 16) == 0);
	res2 = pmem_allocateContiguous(5,16);
	test_assertTrue((res2 % 16) == 0);
	res3 = pmem_allocateContiguous(5,16);
	test_assertTrue((res3 % 16) == 0);
	pmem_freeContiguous(res2,5);
	res2 = pmem_allocateContiguous(3,64);
	test_assertTrue((res2 % 64) == 0);
	res4 = pmem_allocateContiguous(3,64);
	test_assertTrue((res4 % 64) == 0);
	pmem_freeContiguous(res1,5);
	pmem_freeContiguous(res2,3);
	pmem_freeContiguous(res3,5);
	pmem_freeContiguous(res4,3);
	test_assertUInt(pmem_getFreeFrames(MM_CONT),freeContFrames);
	test_caseSucceded();

	test_caseStart("[Align] Request a lot multiple times and free");
	freeContFrames = pmem_getFreeFrames(MM_CONT);
	res1 = pmem_allocateContiguous(35,4);
	test_assertTrue((res1 % 4) == 0);
	res2 = pmem_allocateContiguous(12,4);
	test_assertTrue((res2 % 4) == 0);
	res3 = pmem_allocateContiguous(89,4);
	test_assertTrue((res3 % 4) == 0);
	res4 = pmem_allocateContiguous(56,4);
	test_assertTrue((res4 % 4) == 0);
	pmem_freeContiguous(res3,89);
	pmem_freeContiguous(res1,35);
	pmem_freeContiguous(res2,12);
	pmem_freeContiguous(res4,56);
	test_assertUInt(pmem_getFreeFrames(MM_CONT),freeContFrames);
	test_caseSucceded();
}

static void test_mm_allocate(void) {
	ssize_t i = 0;
	while(i < FRAME_COUNT) {
		frames[i] = pmem_allocate();
		i++;
	}
}

static void test_mm_free(void) {
	ssize_t i = FRAME_COUNT - 1;
	while(i >= 0) {
		pmem_free(frames[i]);
		i--;
	}
}
