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
#include <sys/mem/virtmem.h>
#include <sys/mem/paging.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/video.h>
#include <esc/test.h>
#include "tvirtmem.h"
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
	pid_t cpid;
	Thread *t = Thread::getRunning();
	Proc *p = t->getProc();
	Proc *clone;
	test_caseStart("Testing VirtMem::add() and VirtMem::remove()");

	checkMemoryBefore(true);
	t->reserveFrames(1);
	test_assertTrue(p->getVM()->map(0x1000,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FIXED,NULL,0,&rno) == 0);
	p->getVM()->remove(rno);
	t->discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	t->reserveFrames(9);
	test_assertTrue(p->getVM()->map(0x1000,PAGE_SIZE * 2,PAGE_SIZE * 2,PROT_READ | PROT_EXEC,
			MAP_SHARED | MAP_FIXED,NULL,0,&rno) == 0);
	test_assertTrue(p->getVM()->map(0x8000,PAGE_SIZE * 3,PAGE_SIZE * 3,PROT_READ,
			MAP_PRIVATE | MAP_FIXED,NULL,0,&rno2) == 0);
	test_assertTrue(p->getVM()->map(0x10000,PAGE_SIZE * 4,PAGE_SIZE * 4,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_GROWABLE | MAP_FIXED,NULL,0,&rno3) == 0);
	p->getVM()->remove(rno);
	p->getVM()->remove(rno2);
	p->getVM()->remove(rno3);
	t->discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	t->reserveFrames(8);
	test_assertTrue(p->getVM()->map(0,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,&rno) == 0);
	test_assertTrue(p->getVM()->map(0,PAGE_SIZE * 2,PAGE_SIZE * 2,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,&rno2) == 0);
	test_assertTrue(p->getVM()->map(0,PAGE_SIZE * 5,PAGE_SIZE * 5,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE,NULL,0,&rno3) == 0);
	p->getVM()->remove(rno);
	p->getVM()->remove(rno2);
	p->getVM()->remove(rno3);
	t->discardFrames();
	checkMemoryAfter(true);

	cpid = Proc::clone(0);
	test_assertTrue(cpid > 0);
	clone = Proc::getByPid(cpid);

	checkMemoryBefore(true);
	t->reserveFrames(4);
	test_assertTrue(p->getVM()->map(0,PAGE_SIZE * 4,PAGE_SIZE * 4,PROT_READ,MAP_SHARED,NULL,0,&rno) == 0);
	test_assertTrue(p->getVM()->join(rno->virt,clone->getVM(),&rno2,0) == 0);
	clone->getVM()->remove(rno2);
	p->getVM()->remove(rno);
	t->discardFrames();
	checkMemoryAfter(true);

	Proc::kill(clone->getPid());

	test_caseSucceeded();
}

static void test_2() {
	VMRegion *rno;
	uintptr_t start,end;
	Thread *t = Thread::getRunning();
	Proc *p = t->getProc();
	test_caseStart("Testing VirtMem::grow()");

	checkMemoryBefore(true);
	t->reserveFrames(5);
	test_assertTrue(p->getVM()->map(0x1000,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FIXED | MAP_GROWABLE,NULL,0,&rno) == 0);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt,3),end);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt,-2),end);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt,1),end);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt,-3),end);
	p->getVM()->remove(rno);
	t->discardFrames();
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	t->reserveFrames(7);
	test_assertTrue(p->getVM()->map(0,PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,&rno) == 0);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt,3),start);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt,2),start);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt,-4),start);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt,1),start);
	p->getVM()->getRegRange(rno,&start,&end,true);
	test_assertSSize(p->getVM()->grow(rno->virt,-1),start);
	p->getVM()->remove(rno);
	t->discardFrames();
	checkMemoryAfter(true);

	test_caseSucceeded();
}
