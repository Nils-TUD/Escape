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
#include <sys/mem/kheap.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/video.h>
#include <esc/test.h>
#include "tvmm.h"
#include "testutils.h"

/* forward declarations */
static void test_vmm(void);
static void test_1(void);
static void test_2(void);

/* our test-module */
sTestModule tModVmm = {
	"Virtual Memory Manager",
	&test_vmm
};

static void test_vmm(void) {
	test_1();
	test_2();
}

static void test_1(void) {
	vmreg_t rno,rno2,rno3;
	pid_t cpid;
	pid_t pid = proc_getRunning();
	sProc *clone;
	test_caseStart("Testing vmm_add() and vmm_remove()");

	checkMemoryBefore(true);
	rno = vmm_add(pid,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_DATA);
	test_assertInt(rno,RNO_DATA);
	vmm_remove(pid,rno);
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	rno = vmm_add(pid,NULL,0,PAGE_SIZE * 2,PAGE_SIZE * 2,REG_TEXT);
	test_assertInt(rno,RNO_TEXT);
	rno2 = vmm_add(pid,NULL,0,PAGE_SIZE * 3,PAGE_SIZE * 3,REG_RODATA);
	test_assertInt(rno2,RNO_RODATA);
	rno3 = vmm_add(pid,NULL,0,PAGE_SIZE * 4,PAGE_SIZE * 4,REG_DATA);
	test_assertInt(rno3,RNO_DATA);
	vmm_remove(pid,rno);
	vmm_remove(pid,rno2);
	vmm_remove(pid,rno3);
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	rno = vmm_add(pid,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_STACK);
	test_assertTrue(rno >= 0);
	rno2 = vmm_add(pid,NULL,0,PAGE_SIZE * 2,PAGE_SIZE * 2,REG_STACK);
	test_assertTrue(rno2 >= 0);
	rno3 = vmm_add(pid,NULL,0,PAGE_SIZE * 5,PAGE_SIZE * 5,REG_STACKUP);
	test_assertTrue(rno3 >= 0);
	vmm_remove(pid,rno);
	vmm_remove(pid,rno2);
	vmm_remove(pid,rno3);
	checkMemoryAfter(true);

	cpid = proc_clone(0);
	test_assertTrue(cpid > 0);
	clone = proc_getByPid(cpid);

	checkMemoryBefore(true);
	rno = vmm_add(pid,NULL,0,PAGE_SIZE * 4,PAGE_SIZE * 4,REG_SHM);
	test_assertTrue(rno >= 0);
	rno2 = vmm_join(pid,rno,clone->pid);
	test_assertTrue(rno2 >= 0);
	vmm_remove(clone->pid,rno2);
	vmm_remove(pid,rno);
	checkMemoryAfter(true);

	proc_kill(clone->pid);

	test_caseSucceeded();
}

static void test_2(void) {
	vmreg_t rno;
	uintptr_t start,end;
	pid_t pid = proc_getRunning();
	test_caseStart("Testing vmm_grow()");

	checkMemoryBefore(true);
	rno = vmm_add(pid,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_DATA);
	test_assertTrue(rno >= 0);
	vmm_getRegRange(pid,rno,&start,&end);
	test_assertSSize(vmm_grow(pid,rno,3),end / PAGE_SIZE);
	vmm_getRegRange(pid,rno,&start,&end);
	test_assertSSize(vmm_grow(pid,rno,-2),end / PAGE_SIZE);
	vmm_getRegRange(pid,rno,&start,&end);
	test_assertSSize(vmm_grow(pid,rno,1),end / PAGE_SIZE);
	vmm_getRegRange(pid,rno,&start,&end);
	test_assertSSize(vmm_grow(pid,rno,-3),end / PAGE_SIZE);
	vmm_remove(pid,rno);
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	rno = vmm_add(pid,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_STACK);
	test_assertTrue(rno >= 0);
	vmm_getRegRange(pid,rno,&start,&end);
	test_assertSSize(vmm_grow(pid,rno,3),start / PAGE_SIZE);
	vmm_getRegRange(pid,rno,&start,&end);
	test_assertSSize(vmm_grow(pid,rno,2),start / PAGE_SIZE);
	vmm_getRegRange(pid,rno,&start,&end);
	test_assertSSize(vmm_grow(pid,rno,-4),start / PAGE_SIZE);
	vmm_getRegRange(pid,rno,&start,&end);
	test_assertSSize(vmm_grow(pid,rno,1),start / PAGE_SIZE);
	vmm_getRegRange(pid,rno,&start,&end);
	test_assertSSize(vmm_grow(pid,rno,-1),start / PAGE_SIZE);
	vmm_remove(pid,rno);
	checkMemoryAfter(true);

	test_caseSucceeded();
}
