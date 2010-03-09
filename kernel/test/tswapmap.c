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

#include <common.h>
#include <mem/swapmap.h>
#include <mem/paging.h>
#include <sllist.h>
#include <test.h>

#include "tswapmap.h"

static void test_swapmap(void);
static void test_swapmap1(void);
static void test_swapmap2(void);
static void test_swapmap3(void);
static void test_swapmap4(void);
static void test_swapmap5(void);
static void test_swapmap6(void);
static void test_swapmap7(void);
static void test_swapmap8(void);
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
	test_swapmap7();
	test_swapmap8();
}

static void test_swapmap1(void) {
	u32 blocks[5];
	test_doStart("Testing alloc & free");

	blocks[0] = swmap_alloc(1,NULL,0x00000000,1);
	blocks[1] = swmap_alloc(1,NULL,0x00001000,3);
	blocks[2] = swmap_alloc(1,NULL,0x00005000,1);
	blocks[3] = swmap_alloc(2,NULL,0x00005000,1);
	blocks[4] = swmap_alloc(2,NULL,0x00004000,1);

	test_assertUInt(swmap_find(1,0x00000000),blocks[0]);
	test_assertUInt(swmap_find(1,0x00001000),blocks[1]);
	test_assertUInt(swmap_find(1,0x00002000),blocks[1] + 1);
	test_assertUInt(swmap_find(1,0x00003000),blocks[1] + 2);
	test_assertUInt(swmap_find(2,0x00005000),blocks[3]);
	test_assertUInt(swmap_find(2,0x00004000),blocks[4]);

	swmap_dbg_print();

	swmap_free(1,blocks[0],1);
	swmap_free(1,blocks[1] + 1,1);
	swmap_free(1,blocks[1] + 2,1);
	swmap_free(1,blocks[1],1);
	swmap_free(1,blocks[2],1);
	swmap_free(2,blocks[3],1);
	swmap_free(2,blocks[4],1);

	test_finish();
}

static void test_swapmap2(void) {
	u32 blocks[5];
	test_doStart("Testing alloc & reverse free");

	blocks[0] = swmap_alloc(1,NULL,0x00000000,1);
	blocks[1] = swmap_alloc(1,NULL,0x00001000,2);
	blocks[2] = swmap_alloc(1,NULL,0x00004000,1);
	blocks[3] = swmap_alloc(2,NULL,0x00005000,1);
	blocks[4] = swmap_alloc(2,NULL,0x00004000,1);

	swmap_free(2,blocks[4],1);
	swmap_free(2,blocks[3],1);
	swmap_free(1,blocks[2],1);
	swmap_free(1,blocks[1],2);
	swmap_free(1,blocks[0],1);

	test_finish();
}

static void test_swapmap3(void) {
	test_doStart("Testing alloc & area free");

	swmap_alloc(1,NULL,0x00000000,1);
	swmap_alloc(1,NULL,0x00001000,2);
	swmap_alloc(1,NULL,0x00004000,1);
	swmap_alloc(2,NULL,0x00005000,1);
	swmap_alloc(2,NULL,0x00004000,1);

	swmap_free(1,0,3);
	swmap_free(1,3,1);
	swmap_free(2,5,1);
	swmap_free(2,4,1);

	test_finish();
}

static void test_swapmap4(void) {
	u32 blocks[8];
	test_doStart("Testing alloc & random free");

	blocks[0] = swmap_alloc(1,NULL,0x00000000,1);
	blocks[1] = swmap_alloc(1,NULL,0x00001000,2);
	blocks[2] = swmap_alloc(1,NULL,0x00003000,4);
	blocks[3] = swmap_alloc(2,NULL,0x00000000,1);
	blocks[4] = swmap_alloc(1,NULL,0x00008000,1);
	blocks[5] = swmap_alloc(2,NULL,0x00001000,3);
	blocks[6] = swmap_alloc(2,NULL,0x00004000,1);
	blocks[7] = swmap_alloc(3,NULL,0x00000000,1);

	test_assertUInt(swmap_find(1,0x00000000),blocks[0]);
	test_assertUInt(swmap_find(1,0x00001000),blocks[1]);
	test_assertUInt(swmap_find(1,0x00002000),blocks[1] + 1);
	test_assertUInt(swmap_find(1,0x00003000),blocks[2]);
	test_assertUInt(swmap_find(1,0x00004000),blocks[2] + 1);
	test_assertUInt(swmap_find(1,0x00005000),blocks[2] + 2);
	test_assertUInt(swmap_find(1,0x00006000),blocks[2] + 3);
	test_assertUInt(swmap_find(2,0x00000000),blocks[3]);
	test_assertUInt(swmap_find(1,0x00008000),blocks[4]);
	test_assertUInt(swmap_find(2,0x00001000),blocks[5]);
	test_assertUInt(swmap_find(2,0x00002000),blocks[5] + 1);
	test_assertUInt(swmap_find(2,0x00003000),blocks[5] + 2);
	test_assertUInt(swmap_find(2,0x00004000),blocks[6]);
	test_assertUInt(swmap_find(3,0x00000000),blocks[7]);

	swmap_free(2,blocks[5],2);
	swmap_free(2,blocks[5] + 2,1);
	swmap_free(3,blocks[7],1);
	swmap_free(1,blocks[0],1);
	swmap_free(2,blocks[6],1);
	swmap_free(1,blocks[4],1);
	swmap_free(1,blocks[1],1);
	swmap_free(1,blocks[1] + 1,1);
	swmap_free(1,blocks[2],4);
	swmap_free(2,blocks[3],1);

	test_finish();
}

static void test_swapmap5(void) {
	u32 blocks[8];
	test_doStart("Testing alloc & free mixed");

	blocks[0] = swmap_alloc(1,NULL,0x00000000,2);
	blocks[1] = swmap_alloc(1,NULL,0x00002000,1);
	blocks[2] = swmap_alloc(2,NULL,0x00004000,1);
	blocks[3] = swmap_alloc(1,NULL,0x00004000,1);

	test_assertUInt(swmap_find(1,0x00000000),blocks[0]);
	test_assertUInt(swmap_find(1,0x00001000),blocks[0] + 1);
	test_assertUInt(swmap_find(1,0x00002000),blocks[1]);
	test_assertUInt(swmap_find(2,0x00004000),blocks[2]);
	test_assertUInt(swmap_find(1,0x00004000),blocks[3]);

	swmap_free(2,blocks[2],1);

	test_assertUInt(swmap_find(1,0x00000000),blocks[0]);
	test_assertUInt(swmap_find(1,0x00001000),blocks[0] + 1);
	test_assertUInt(swmap_find(1,0x00002000),blocks[1]);
	test_assertUInt(swmap_find(2,0x00004000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00004000),blocks[3]);

	blocks[4] = swmap_alloc(1,NULL,0x00003000,1);
	blocks[5] = swmap_alloc(2,NULL,0x00000000,1);
	blocks[6] = swmap_alloc(1,NULL,0x00008000,1);
	blocks[7] = swmap_alloc(2,NULL,0x00003000,1);

	test_assertUInt(swmap_find(1,0x00003000),blocks[4]);
	test_assertUInt(swmap_find(2,0x00000000),blocks[5]);
	test_assertUInt(swmap_find(1,0x00008000),blocks[6]);
	test_assertUInt(swmap_find(2,0x00003000),blocks[7]);

	swmap_free(1,blocks[6],1);

	test_assertUInt(swmap_find(1,0x00003000),blocks[4]);
	test_assertUInt(swmap_find(2,0x00000000),blocks[5]);
	test_assertUInt(swmap_find(1,0x00008000),INVALID_BLOCK);
	test_assertUInt(swmap_find(2,0x00003000),blocks[7]);

	blocks[8] = swmap_alloc(2,NULL,0x00002000,1);

	test_assertUInt(swmap_find(2,0x00002000),blocks[8]);

	swmap_free(2,blocks[5],1);
	swmap_free(2,blocks[8],2);
	swmap_free(1,blocks[0],5);

	test_finish();
}

static void test_swapmap6(void) {
	u32 addr;
	test_doStart("Testing alloc all & free");

	addr = 0;
	while(swmap_freeSpace() > 1 * PAGE_SIZE) {
		swmap_alloc(1,NULL,addr,1);
		addr += PAGE_SIZE;
	}
	swmap_free(1,0,addr / PAGE_SIZE);

	test_finish();
}

static void test_swapmap7(void) {
	u32 block;
	test_doStart("Testing free one region in multiple steps in random order");

	block = swmap_alloc(1,NULL,0x00000000,6);
	swmap_free(1,block + 3,1);

	test_assertUInt(swmap_find(1,0x00000000),block + 0);
	test_assertUInt(swmap_find(1,0x00001000),block + 1);
	test_assertUInt(swmap_find(1,0x00002000),block + 2);
	test_assertUInt(swmap_find(1,0x00003000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00004000),block + 4);
	test_assertUInt(swmap_find(1,0x00005000),block + 5);

	swmap_free(1,block + 0,1);

	test_assertUInt(swmap_find(1,0x00000000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00001000),block + 1);
	test_assertUInt(swmap_find(1,0x00002000),block + 2);
	test_assertUInt(swmap_find(1,0x00003000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00004000),block + 4);
	test_assertUInt(swmap_find(1,0x00005000),block + 5);

	swmap_free(1,block + 5,1);

	test_assertUInt(swmap_find(1,0x00000000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00001000),block + 1);
	test_assertUInt(swmap_find(1,0x00002000),block + 2);
	test_assertUInt(swmap_find(1,0x00003000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00004000),block + 4);
	test_assertUInt(swmap_find(1,0x00005000),INVALID_BLOCK);

	swmap_free(1,block + 1,1);

	test_assertUInt(swmap_find(1,0x00000000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00001000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00002000),block + 2);
	test_assertUInt(swmap_find(1,0x00003000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00004000),block + 4);
	test_assertUInt(swmap_find(1,0x00005000),INVALID_BLOCK);

	swmap_free(1,block + 4,1);

	test_assertUInt(swmap_find(1,0x00000000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00001000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00002000),block + 2);
	test_assertUInt(swmap_find(1,0x00003000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00004000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00005000),INVALID_BLOCK);

	swmap_free(1,block + 2,1);

	test_assertUInt(swmap_find(1,0x00000000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00001000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00002000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00003000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00004000),INVALID_BLOCK);
	test_assertUInt(swmap_find(1,0x00005000),INVALID_BLOCK);

	test_finish();
}

static void test_swapmap8(void) {
	u32 block1,block2;
	sSLList *procs = sll_create();
	sProc *p1,*p2,*p3;
	test_doStart("Testing swmap_remProc()");

	/* we have to "create" a few processes here */
	test_assertTrue(procs != NULL);
	p1 = proc_getByPid(0);
	p2 = proc_getByPid(1);
	p2->pid = 1;
	p3 = proc_getByPid(2);
	p3->pid = 2;
	test_assertTrue(sll_append(procs,p1));
	test_assertTrue(sll_append(procs,p2));
	test_assertTrue(sll_append(procs,p3));

	block1 = swmap_alloc(p2->pid,procs,0x00000000,6);
	block2 = swmap_alloc(p2->pid,NULL,0x00006000,4);

	swmap_remProc(p2->pid,procs);
	test_assertUInt(swmap_find(p2->pid,0x00000000),block1);
	test_assertUInt(swmap_find(p2->pid,0x00006000),block2);
	sll_removeFirst(procs,p2);

	swmap_remProc(p3->pid,procs);
	test_assertUInt(swmap_find(p2->pid,0x00000000),block1);
	test_assertUInt(swmap_find(p2->pid,0x00006000),block2);
	sll_removeFirst(procs,p3);

	swmap_remProc(p2->pid,NULL);
	test_assertUInt(swmap_find(p2->pid,0x00000000),block1);
	test_assertUInt(swmap_find(p2->pid,0x00006000),INVALID_BLOCK);

	swmap_remProc(p1->pid,procs);
	test_assertUInt(swmap_find(p2->pid,0x00000000),INVALID_BLOCK);
	test_assertUInt(swmap_find(p2->pid,0x00006000),INVALID_BLOCK);
	sll_removeFirst(procs,p1);

	test_finish();
	p3->pid = INVALID_PID;
	p2->pid = INVALID_PID;
	sll_destroy(procs,false);
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
