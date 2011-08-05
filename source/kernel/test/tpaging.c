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
#include <sys/mem/paging.h>
#include <sys/task/proc.h>
#include <sys/mem/pmem.h>
#include <sys/video.h>
#include <string.h>
#include <esc/test.h>
#include "tpaging.h"
#include "testutils.h"

/* forward declarations */
static void test_paging(void);
static void test_paging_foreign(void);
static bool test_paging_cycle(uintptr_t addr,size_t count);
static void test_paging_allocate(uintptr_t addr,size_t count);
static void test_paging_access(uintptr_t addr,size_t count);
static void test_paging_free(uintptr_t addr,size_t count);

/* our test-module */
sTestModule tModPaging = {
	"Paging",
	&test_paging
};

static void test_paging(void) {
	size_t x,y;
	uintptr_t addr[] = {
		0x0,0x40000000,0x70000000,0x4000,0x1234
	};
	size_t count[] = {0,1,50,1024,1025,2048,2051};

	for(y = 0; y < ARRAY_SIZE(addr); y++) {
		for(x = 0; x < ARRAY_SIZE(count); x++) {
			test_paging_cycle(addr[y],count[x]);
		}
	}

	test_paging_foreign();
}

static void test_paging_foreign(void) {
	size_t ownFrames, sharedFrames;
	sProc *child;
	pid_t pid = proc_getFreePid();
	test_assertInt(proc_clone(pid,0),0);
	child = proc_getByPid(pid);

	test_caseStart("Mapping %d pages to %p into pdir %p",3,0,&child->pagedir);
	ownFrames = child->ownFrames;
	sharedFrames = child->sharedFrames;
	checkMemoryBefore(true);

	paging_mapTo(&child->pagedir,0,NULL,3,PG_PRESENT | PG_WRITABLE);
	paging_unmapFrom(&child->pagedir,0,3,true);

	checkMemoryAfter(true);
	if(child->ownFrames != ownFrames || child->sharedFrames != sharedFrames) {
		test_caseFailed("oldOwn=%zu, newOwn=%zu, oldSh=%zu, newSh=%zu",
				ownFrames,child->ownFrames,sharedFrames,child->sharedFrames);
	}
	else
		test_caseSucceeded();

	test_caseStart("Mapping %d pages to %p into pdir %p, separatly",6,0x40000000,&child->pagedir);
	ownFrames = child->ownFrames;
	sharedFrames = child->sharedFrames;
	checkMemoryBefore(true);

	paging_mapTo(&child->pagedir,0x40000000,NULL,3,PG_PRESENT | PG_WRITABLE);
	paging_mapTo(&child->pagedir,0x40000000 + PAGE_SIZE * 3,NULL,3,PG_PRESENT | PG_WRITABLE);
	paging_unmapFrom(&child->pagedir,0x40000000,1,true);
	paging_unmapFrom(&child->pagedir,0x40000000 + PAGE_SIZE * 1,5,true);

	checkMemoryAfter(true);
	if(child->ownFrames != ownFrames || child->sharedFrames != sharedFrames) {
		test_caseFailed("oldOwn=%zu, newOwn=%zu, oldSh=%zu, newSh=%zu",
				ownFrames,child->ownFrames,sharedFrames,child->sharedFrames);
	}
	else
		test_caseSucceeded();
	proc_kill(child->pid);
}

static bool test_paging_cycle(uintptr_t addr,size_t count) {
	test_caseStart("Mapping %zu pages to %p",count,addr);
	checkMemoryBefore(true);

	test_paging_allocate(addr,count);
	test_paging_access(addr,count);
	test_paging_free(addr,count);

	checkMemoryAfter(true);

	test_caseSucceeded();
	return true;
}

static void test_paging_allocate(uintptr_t addr,size_t count) {
	paging_map(addr,NULL,count,PG_PRESENT | PG_WRITABLE);
}

static void test_paging_access(uintptr_t addr,size_t count) {
	size_t i;
	addr &= ~(PAGE_SIZE - 1);
	for(i = 0; i < count; i++) {
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
	paging_unmap(addr,count,true);
}
