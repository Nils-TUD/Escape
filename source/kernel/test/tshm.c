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
#include <sys/mem/sharedmem.h>
#include <sys/mem/vmm.h>
#include <sys/mem/pmem.h>
#include <sys/video.h>
#include <esc/test.h>
#include "tshm.h"
#include "testutils.h"

/* forward declarations */
static void test_shm(void);
static void test_1(void);
static void test_2(void);

/* our test-module */
sTestModule tModShm = {
	"Shared memory",
	&test_shm
};

static void test_shm(void) {
	test_1();
	test_2();
}

static void test_1(void) {
	sProc *p = proc_getRunning();
	test_caseStart("Testing shm_create() & shm_destroy()");
	checkMemoryBefore(true);

	test_assertTrue(shm_create(p,"myshm",3) >= 0);
	test_assertTrue(shm_destroy(p,"myshm") == 0);

	checkMemoryAfter(true);
	test_caseSucceeded();
}

static void test_2(void) {
	sProc *p = proc_getRunning();
	sProc *child1,*child2;
	pid_t pid1,pid2;
	vmreg_t reg1,reg2;
	test_caseStart("Testing shm_join() & shm_leave() & shm_remProc()");

	pid1 = proc_getFreePid();
	test_assertInt(proc_clone(pid1,0),0);
	child1 = proc_getByPid(pid1);
	pid2 = proc_getFreePid();
	test_assertInt(proc_clone(pid2,0),0);
	child2 = proc_getByPid(pid2);

	/* create dummy-regions to force vmm to extend the regions-array. this way we can check
	 * whether all memory is freed correctly */
	checkMemoryBefore(true);
	reg1 = vmm_add(child1,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM);
	test_assertTrue(reg1 >= 0);
	reg2 = vmm_add(child2,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM);
	test_assertTrue(reg2 >= 0);
	test_assertTrue(shm_create(p,"myshm",3) >= 0);
	test_assertTrue(shm_join(child1,"myshm") >= 0);
	test_assertTrue(shm_join(child2,"myshm") >= 0);
	test_assertTrue(shm_leave(child1,"myshm") >= 0);
	test_assertTrue(shm_leave(child2,"myshm") >= 0);
	test_assertTrue(shm_destroy(p,"myshm") == 0);
	vmm_remove(child2,reg2);
	vmm_remove(child1,reg1);
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	reg1 = vmm_add(child1,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM);
	test_assertTrue(reg1 >= 0);
	reg2 = vmm_add(child2,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM);
	test_assertTrue(reg2 >= 0);
	test_assertTrue(shm_create(p,"myshm",3) >= 0);
	test_assertTrue(shm_join(child1,"myshm") >= 0);
	test_assertTrue(shm_join(child2,"myshm") >= 0);
	test_assertTrue(shm_destroy(p,"myshm") == 0);
	vmm_remove(child2,reg2);
	vmm_remove(child1,reg1);
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	reg1 = vmm_add(child1,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM);
	test_assertTrue(reg1 >= 0);
	reg2 = vmm_add(child2,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM);
	test_assertTrue(reg2 >= 0);
	test_assertTrue(shm_create(p,"myshm",6) >= 0);
	test_assertTrue(shm_join(child1,"myshm") >= 0);
	test_assertTrue(shm_join(child2,"myshm") >= 0);
	shm_remProc(child2);
	shm_remProc(p);
	vmm_remove(child2,reg2);
	vmm_remove(child1,reg1);
	checkMemoryAfter(true);

	proc_kill(child1);
	proc_kill(child2);

	test_caseSucceeded();
}
