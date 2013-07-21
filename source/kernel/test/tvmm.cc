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
#include <sys/task/thread.h>
#include <sys/task/proc.h>
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
	VMRegion *rno,*rno2,*rno3;
	pid_t cpid;
	Thread *t = Thread::getRunning();
	pid_t pid = t->getProc()->getPid();
	Proc *clone;
	test_caseStart("Testing vmm_add() and vmm_remove()");

	checkMemoryBefore(true);
	t->reserveFrames(1);
	test_assertTrue(vmm_map(pid,0x1000,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FIXED,NULL,0,&rno) == 0);
	vmm_remove(pid,rno);
	t->discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	t->reserveFrames(9);
	test_assertTrue(vmm_map(pid,0x1000,PAGE_SIZE * 2,PAGE_SIZE * 2,PROT_READ | PROT_EXEC,
			MAP_SHARED | MAP_FIXED,NULL,0,&rno) == 0);
	test_assertTrue(vmm_map(pid,0x8000,PAGE_SIZE * 3,PAGE_SIZE * 3,PROT_READ,
			MAP_PRIVATE | MAP_FIXED,NULL,0,&rno2) == 0);
	test_assertTrue(vmm_map(pid,0x10000,PAGE_SIZE * 4,PAGE_SIZE * 4,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_GROWABLE | MAP_FIXED,NULL,0,&rno3) == 0);
	vmm_remove(pid,rno);
	vmm_remove(pid,rno2);
	vmm_remove(pid,rno3);
	t->discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	t->reserveFrames(8);
	test_assertTrue(vmm_map(pid,0,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,&rno) == 0);
	test_assertTrue(vmm_map(pid,0,PAGE_SIZE * 2,PAGE_SIZE * 2,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,&rno2) == 0);
	test_assertTrue(vmm_map(pid,0,PAGE_SIZE * 5,PAGE_SIZE * 5,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE,NULL,0,&rno3) == 0);
	vmm_remove(pid,rno);
	vmm_remove(pid,rno2);
	vmm_remove(pid,rno3);
	t->discardFrames();
	checkMemoryAfter(true);

	cpid = Proc::clone(0);
	test_assertTrue(cpid > 0);
	clone = Proc::getByPid(cpid);

	checkMemoryBefore(true);
	t->reserveFrames(4);
	test_assertTrue(vmm_map(pid,0,PAGE_SIZE * 4,PAGE_SIZE * 4,PROT_READ,MAP_SHARED,NULL,0,&rno) == 0);
	test_assertTrue(vmm_join(pid,rno->virt,clone->getPid(),&rno2,0) == 0);
	vmm_remove(clone->getPid(),rno2);
	vmm_remove(pid,rno);
	t->discardFrames();
	checkMemoryAfter(true);

	Proc::kill(clone->getPid());

	test_caseSucceeded();
}

static void test_2(void) {
	VMRegion *rno;
	uintptr_t start,end;
	Thread *t = Thread::getRunning();
	pid_t pid = t->getProc()->getPid();
	test_caseStart("Testing vmm_grow()");

	checkMemoryBefore(true);
	t->reserveFrames(5);
	test_assertTrue(vmm_map(pid,0x1000,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FIXED | MAP_GROWABLE,NULL,0,&rno) == 0);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,3),end);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,-2),end);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,1),end);
	vmm_getRegRange(pid,rno,&start,&end,true);
	test_assertSSize(vmm_grow(pid,rno->virt,-3),end);
	vmm_remove(pid,rno);
	t->discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	t->reserveFrames(7);
	test_assertTrue(vmm_map(pid,0,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,&rno) == 0);
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
	t->discardFrames();
	checkMemoryAfter(true);

	test_caseSucceeded();
}
