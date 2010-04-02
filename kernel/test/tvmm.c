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
#include <mem/kheap.h>
#include <mem/vmm.h>
#include <mem/paging.h>
#include <video.h>
#include <test.h>
#include "tvmm.h"

/* forward declarations */
static void test_vmm(void);
static void test_1(void);
static void test_2(void);

/* our test-module */
sTestModule tModVmm = {
	"Virtual Memory Manager",
	&test_vmm
};
static u32 heapBefore;
static u32 framesBefore;
static u32 heapAfter;
static u32 framesAfter;

static void test_init(void) {
	heapBefore = kheap_getFreeMem();
	framesBefore = mm_getFreeFrmCount(MM_DEF);
}
static void test_finish(void) {
	heapAfter = kheap_getFreeMem();
	framesAfter = mm_getFreeFrmCount(MM_DEF);
	test_assertTrue(heapAfter >= heapBefore);
	test_assertUInt(framesAfter,framesBefore);
}

static void test_vmm(void) {
	test_1();
	test_2();
}

static void test_1(void) {
	tVMRegNo rno,rno2;
	tPid pid;
	sProc *p = proc_getRunning();
	sProc *clone;
	test_caseStart("Testing vmm_add() and vmm_remove()");

	test_init();
	rno = vmm_add(p,NULL,0,PAGE_SIZE,REG_DATA);
	test_assertInt(rno,RNO_DATA);
	vmm_remove(p,rno);
	test_finish();

	test_init();
	rno = vmm_add(p,NULL,0,PAGE_SIZE * 2,REG_TEXT);
	test_assertInt(rno,RNO_TEXT);
	rno = vmm_add(p,NULL,0,PAGE_SIZE * 3,REG_RODATA);
	test_assertInt(rno,RNO_RODATA);
	rno = vmm_add(p,NULL,0,PAGE_SIZE * 4,REG_BSS);
	test_assertInt(rno,RNO_BSS);
	rno = vmm_add(p,NULL,0,PAGE_SIZE * 5,REG_DATA);
	test_assertInt(rno,RNO_DATA);
	vmm_removeAll(p,false);
	test_finish();

	test_init();
	rno = vmm_add(p,NULL,0,PAGE_SIZE,REG_STACK);
	test_assertTrue(rno >= 0);
	rno2 = vmm_add(p,NULL,0,PAGE_SIZE * 2,REG_STACK);
	test_assertTrue(rno2 >= 0);
	vmm_remove(p,rno);
	vmm_remove(p,rno2);
	test_finish();

	pid = proc_getFreePid();
	test_assertUInt(proc_clone(pid,false),0);
	clone = proc_getByPid(pid);

	test_init();
	rno = vmm_add(p,NULL,0,PAGE_SIZE * 4,REG_SHM);
	test_assertTrue(rno >= 0);
	rno2 = vmm_join(p,rno,clone);
	test_assertTrue(rno2 >= 0);
	vmm_remove(clone,rno2);
	vmm_remove(p,rno);
	test_finish();

	proc_kill(clone);

	test_caseSucceded();
}

static void test_2(void) {
	tVMRegNo rno;
	u32 start,end;
	sProc *p = proc_getRunning();
	test_caseStart("Testing vmm_grow()");

	test_init();
	rno = vmm_add(p,NULL,0,PAGE_SIZE,REG_DATA);
	test_assertTrue(rno >= 0);
	vmm_getRegRange(p,rno,&start,&end);
	test_assertInt(vmm_grow(p,rno,3),end / PAGE_SIZE);
	vmm_getRegRange(p,rno,&start,&end);
	test_assertInt(vmm_grow(p,rno,-2),end / PAGE_SIZE);
	vmm_getRegRange(p,rno,&start,&end);
	test_assertInt(vmm_grow(p,rno,1),end / PAGE_SIZE);
	vmm_getRegRange(p,rno,&start,&end);
	test_assertInt(vmm_grow(p,rno,-3),end / PAGE_SIZE);
	vmm_remove(p,rno);
	test_finish();

	test_init();
	rno = vmm_add(p,NULL,0,PAGE_SIZE,REG_STACK);
	test_assertTrue(rno >= 0);
	vmm_getRegRange(p,rno,&start,&end);
	test_assertInt(vmm_grow(p,rno,3),start / PAGE_SIZE);
	vmm_getRegRange(p,rno,&start,&end);
	test_assertInt(vmm_grow(p,rno,2),start / PAGE_SIZE);
	vmm_getRegRange(p,rno,&start,&end);
	test_assertInt(vmm_grow(p,rno,-4),start / PAGE_SIZE);
	vmm_getRegRange(p,rno,&start,&end);
	test_assertInt(vmm_grow(p,rno,1),start / PAGE_SIZE);
	vmm_getRegRange(p,rno,&start,&end);
	test_assertInt(vmm_grow(p,rno,-1),start / PAGE_SIZE);
	vmm_remove(p,rno);
	test_finish();

	test_caseSucceded();
}
