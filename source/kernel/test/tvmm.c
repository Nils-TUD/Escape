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
	sVMRegion *rno,*rno2,*rno3;
	pid_t cpid;
	sThread *t = thread_getRunning();
	pid_t pid = t->proc->pid;
	sProc *clone;
	test_caseStart("Testing vmm_add() and vmm_remove()");

	checkMemoryBefore(true);
	thread_reserveFrames(1);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_DATA,&rno,0) == 0);
	vmm_remove(pid,rno);
	thread_discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	thread_reserveFrames(9);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE * 2,PAGE_SIZE * 2,REG_TEXT,&rno,0) == 0);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE * 3,PAGE_SIZE * 3,REG_RODATA,&rno2,0) == 0);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE * 4,PAGE_SIZE * 4,REG_DATA,&rno3,0) == 0);
	vmm_remove(pid,rno);
	vmm_remove(pid,rno2);
	vmm_remove(pid,rno3);
	thread_discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	thread_reserveFrames(8);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_STACK,&rno,0) == 0);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE * 2,PAGE_SIZE * 2,REG_STACK,&rno2,0) == 0);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE * 5,PAGE_SIZE * 5,REG_STACKUP,&rno3,0) == 0);
	vmm_remove(pid,rno);
	vmm_remove(pid,rno2);
	vmm_remove(pid,rno3);
	thread_discardFrames();
	checkMemoryAfter(true);

	cpid = proc_clone(0);
	test_assertTrue(cpid > 0);
	clone = proc_getByPid(cpid);

	checkMemoryBefore(true);
	thread_reserveFrames(4);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE * 4,PAGE_SIZE * 4,REG_SHM,&rno,0) == 0);
	test_assertTrue(vmm_join(pid,rno->virt,clone->pid,&rno2,0) == 0);
	vmm_remove(clone->pid,rno2);
	vmm_remove(pid,rno);
	thread_discardFrames();
	checkMemoryAfter(true);

	proc_kill(clone->pid);

	test_caseSucceeded();
}

static void test_2(void) {
	sVMRegion *rno;
	uintptr_t start,end;
	sThread *t = thread_getRunning();
	pid_t pid = t->proc->pid;
	test_caseStart("Testing vmm_grow()");

	checkMemoryBefore(true);
	thread_reserveFrames(5);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_DATA,&rno,0) == 0);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,3),end);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,-2),end);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,1),end);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,-3),end);
	vmm_remove(pid,rno);
	thread_discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	thread_reserveFrames(7);
	test_assertTrue(vmm_add(pid,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_STACK,&rno,0) == 0);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,3),start);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,2),start);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,-4),start);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,1),start);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,-1),start);
	vmm_remove(pid,rno);
	thread_discardFrames();
	checkMemoryAfter(true);

	test_caseSucceeded();
}
