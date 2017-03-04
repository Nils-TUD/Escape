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

#include <mem/kheap.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <sys/test.h>
#include <task/proc.h>
#include <task/thread.h>
#include <common.h>
#include <video.h>

#include "testutils.h"

/* forward declarations */
static void test_vmm();
static void test_1();
static void test_2();

/* our test-module */
sTestModule tModVmm = {
	"Virtual Memory Manager",
	&test_vmm
};

static void test_vmm() {
	test_1();
	test_2();
}

static void test_1() {
	VMRegion *rno,*rno2,*rno3;
	uintptr_t addr;
	Thread *t = Thread::getRunning();
	Proc *p = t->getProc();
	test_caseStart("Testing VirtMem::add() and VirtMem::unmap()");

	checkMemoryBefore(true);
	t->reserveFrames(1);
	addr = 0x1000;
	test_assertTrue(p->getVM()->map(&addr,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FIXED,NULL,0,&rno) == 0);
	p->getVM()->unmap(rno);
	t->discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	t->reserveFrames(9);
	addr = 0x1000;
	test_assertTrue(p->getVM()->map(&addr,PAGE_SIZE * 2,PAGE_SIZE * 2,PROT_READ | PROT_EXEC,
			MAP_SHARED | MAP_FIXED,NULL,0,&rno) == 0);
	addr = 0x8000;
	test_assertTrue(p->getVM()->map(&addr,PAGE_SIZE * 3,PAGE_SIZE * 3,PROT_READ,
			MAP_PRIVATE | MAP_FIXED,NULL,0,&rno2) == 0);
	addr = 0x10000;
	test_assertTrue(p->getVM()->map(&addr,PAGE_SIZE * 4,PAGE_SIZE * 4,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_GROWABLE | MAP_FIXED,NULL,0,&rno3) == 0);
	p->getVM()->unmap(rno);
	p->getVM()->unmap(rno2);
	p->getVM()->unmap(rno3);
	t->discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	t->reserveFrames(8);
	test_assertTrue(p->getVM()->map(NULL,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,&rno) == 0);
	test_assertTrue(p->getVM()->map(NULL,PAGE_SIZE * 2,PAGE_SIZE * 2,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,&rno2) == 0);
	test_assertTrue(p->getVM()->map(NULL,PAGE_SIZE * 5,PAGE_SIZE * 5,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE,NULL,0,&rno3) == 0);
	p->getVM()->unmap(rno);
	p->getVM()->unmap(rno2);
	p->getVM()->unmap(rno3);
	t->discardFrames();
	checkMemoryAfter(true);

	/* doesn't work on mmix since we would have to leave the kernel and enter it again in order to
	 * get a new kernel-stack */
#ifndef __mmix__
	pid_t cpid = Proc::clone(0);
	if(cpid == 0) {
		Proc::terminate(0);
		A_UNREACHED;
	}
	test_assertTrue(cpid > 0);
	Proc *clone = Proc::getByPid(cpid);

	checkMemoryBefore(true);
	t->reserveFrames(4);
	test_assertTrue(p->getVM()->map(NULL,PAGE_SIZE * 4,PAGE_SIZE * 4,PROT_READ,MAP_SHARED,NULL,0,&rno) == 0);
	test_assertTrue(p->getVM()->join(rno->virt(),clone->getVM(),&rno2,0,0) == 0);
	clone->getVM()->unmap(rno2);
	p->getVM()->unmap(rno);
	t->discardFrames();
	checkMemoryAfter(true);

	Proc::waitChild(NULL,-1,0);
	test_assertTrue(Proc::getByPid(cpid) == NULL);
#endif

	test_caseSucceeded();
}

static void test_2() {
	VMRegion *rno;
	uintptr_t addr,start,end;
	Thread *t = Thread::getRunning();
	Proc *p = t->getProc();
	test_caseStart("Testing VirtMem::grow()");

	checkMemoryBefore(true);
	t->reserveFrames(5);
	addr = 0x1000;
	test_assertTrue(p->getVM()->map(&addr,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FIXED | MAP_GROWABLE,NULL,0,&rno) == 0);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt(),3),end);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt(),-2),end);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt(),1),end);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt(),-3),end);
	p->getVM()->unmap(rno);
	t->discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	t->reserveFrames(7);
	test_assertTrue(p->getVM()->map(NULL,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,&rno) == 0);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt(),3),start);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt(),2),start);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt(),-4),start);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt(),1),start);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt(),-1),start);
	p->getVM()->unmap(rno);
	t->discardFrames();
	checkMemoryAfter(true);

	test_caseSucceeded();
}
