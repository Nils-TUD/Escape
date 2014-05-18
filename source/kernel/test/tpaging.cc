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

#include <sys/common.h>
#include <sys/mem/pagedir.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/mem/physmem.h>
#include <sys/video.h>
#include <esc/test.h>
#include <assert.h>
#include <string.h>
#include "testutils.h"

/* forward declarations */
static void test_paging();
static void test_paging_foreign();
static bool test_paging_cycle(uintptr_t addr,size_t count);
static void test_paging_allocate(uintptr_t addr,size_t count);
static void test_paging_access(uintptr_t addr,size_t count);
static void test_paging_free(uintptr_t addr,size_t count);

/* our test-module */
sTestModule tModPaging = {
	"Paging",
	&test_paging
};

static void test_paging() {
	uintptr_t addr[] = {
		0x0,0x40000000,0x70000000,0x4000,0x1234
	};
	size_t count[] = {0,1,50,1024,1025,2048,2051};

	for(size_t y = 0; y < ARRAY_SIZE(addr); y++) {
		for(size_t x = 0; x < ARRAY_SIZE(count); x++) {
			test_paging_cycle(addr[y],count[x]);
		}
	}

	test_paging_foreign();
}

static void test_paging_foreign() {
	size_t ownFrames, sharedFrames;
	Proc *child;
	PageDir::UAllocator alloc;
	Thread *t = Thread::getRunning();
	pid_t pid = Proc::clone(0);
	if(pid == 0) {
		Proc::terminate(0);
		A_UNREACHED;
	}

	test_assertTrue(pid > 0);
	child = Proc::getByPid(pid);

	test_caseStart("Mapping %d pages to %p into pdir %p",3,0,child->getPageDir());
	ownFrames = child->getVM()->getOwnFrames();
	sharedFrames = child->getVM()->getSharedFrames();
	checkMemoryBefore(true);

	test_assertTrue(t->reserveFrames(3));
	test_assertTrue(child->getPageDir()->map(0,3,alloc,PG_PRESENT | PG_WRITABLE) >= 0);
	child->getPageDir()->unmap(0,3,alloc);
	t->discardFrames();

	checkMemoryAfter(true);
	if(child->getVM()->getOwnFrames() != ownFrames || child->getVM()->getSharedFrames() != sharedFrames) {
		test_caseFailed("oldOwn=%zu, newOwn=%zu, oldSh=%zu, newSh=%zu",
				ownFrames,child->getVM()->getOwnFrames(),sharedFrames,child->getVM()->getSharedFrames());
	}
	else
		test_caseSucceeded();

	test_caseStart("Mapping %d pages to %p into pdir %p, separatly",6,0x40000000,child->getPageDir());
	ownFrames = child->getVM()->getOwnFrames();
	sharedFrames = child->getVM()->getSharedFrames();
	checkMemoryBefore(true);

	test_assertTrue(t->reserveFrames(6));
	test_assertTrue(child->getPageDir()->map(0x40000000,3,alloc,PG_PRESENT | PG_WRITABLE) >= 0);
	test_assertTrue(child->getPageDir()->map(0x40000000 + PAGE_SIZE * 3,3,alloc,PG_PRESENT | PG_WRITABLE) >= 0);
	child->getPageDir()->unmap(0x40000000,1,alloc);
	child->getPageDir()->unmap(0x40000000 + PAGE_SIZE * 1,5,alloc);
	t->discardFrames();

	checkMemoryAfter(true);
	if(child->getVM()->getOwnFrames() != ownFrames || child->getVM()->getSharedFrames() != sharedFrames) {
		test_caseFailed("oldOwn=%zu, newOwn=%zu, oldSh=%zu, newSh=%zu",
				ownFrames,child->getVM()->getOwnFrames(),sharedFrames,child->getVM()->getSharedFrames());
	}
	else
		test_caseSucceeded();

	/* doesn't work on mmix since we would have to leave the kernel and enter it again in order to
	 * get a new kernel-stack */
#ifndef __mmix__
	/* give the process a chance to terminate */
	Proc::waitChild(NULL);
	test_assertTrue(Proc::getByPid(pid) == NULL);
#endif
}

static bool test_paging_cycle(uintptr_t addr,size_t count) {
	Thread *t = Thread::getRunning();
	test_caseStart("Mapping %zu pages to %p",count,addr);
	checkMemoryBefore(true);

	test_assertTrue(t->reserveFrames(count));
	test_paging_allocate(addr,count);
	test_paging_access(addr,count);
	test_paging_free(addr,count);
	t->discardFrames();

	checkMemoryAfter(true);

	test_caseSucceeded();
	return true;
}

static void test_paging_allocate(uintptr_t addr,size_t count) {
	PageDir::UAllocator alloc;
	PageDir::mapToCur(addr,count,alloc,PG_PRESENT | PG_WRITABLE);
}

static void test_paging_access(uintptr_t addr,size_t count) {
	addr &= ~(PAGE_SIZE - 1);
	for(size_t i = 0; i < count; i++) {
		/* write to the first word */
		*(uint*)addr = 0xDEADBEEF;
		test_assertUInt(*(uint*)addr,0xDEADBEEF);
		/* write to the last word */
		*(uint*)(addr + PAGE_SIZE - sizeof(uint)) = 0xDEADBEEF;
		test_assertUInt(*(uint*)(addr + PAGE_SIZE - sizeof(uint)),0xDEADBEEF);
		addr += PAGE_SIZE;
	}
}

static void test_paging_free(uintptr_t addr,size_t count) {
	PageDir::UAllocator alloc;
	PageDir::unmapFromCur(addr,count,alloc);
}
