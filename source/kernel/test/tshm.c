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
	pid_t pid = proc_getRunning();
	test_caseStart("Testing shm_create() & shm_destroy()");
	checkMemoryBefore(true);

	test_assertTrue(shm_create(pid,"myshm",3) >= 0);
	test_assertTrue(shm_destroy(pid,"myshm") == 0);

	checkMemoryAfter(true);
	test_caseSucceeded();
}

static void test_2(void) {
	pid_t pid = proc_getRunning();
	pid_t pid1,pid2;
	sVMRegion *reg1,*reg2;
	test_caseStart("Testing shm_join() & shm_leave() & shm_remProc()");

	pid1 = proc_clone(0);
	test_assertTrue(pid1 > 0);
	pid2 = proc_clone(0);
	test_assertTrue(pid2 > 0);

	/* create dummy-regions to force vmm to extend the regions-array. this way we can check
	 * whether all memory is freed correctly */
	checkMemoryBefore(true);
	thread_reserveFrames(2);
	test_assertTrue(vmm_add(pid1,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM,&reg1) == 0);
	test_assertTrue(vmm_add(pid2,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM,&reg2) == 0);
	thread_discardFrames();
	test_assertTrue(shm_create(pid,"myshm",3) >= 0);
	test_assertTrue(shm_join(pid1,"myshm") >= 0);
	test_assertTrue(shm_join(pid2,"myshm") >= 0);
	test_assertTrue(shm_leave(pid1,"myshm") >= 0);
	test_assertTrue(shm_leave(pid2,"myshm") >= 0);
	test_assertTrue(shm_destroy(pid,"myshm") == 0);
	vmm_remove(pid2,reg2);
	vmm_remove(pid1,reg1);
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	thread_reserveFrames(2);
	test_assertTrue(vmm_add(pid1,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM,&reg1) == 0);
	test_assertTrue(vmm_add(pid2,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM,&reg2) == 0);
	thread_discardFrames();
	test_assertTrue(shm_create(pid,"myshm",3) >= 0);
	test_assertTrue(shm_join(pid1,"myshm") >= 0);
	test_assertTrue(shm_join(pid2,"myshm") >= 0);
	test_assertTrue(shm_destroy(pid,"myshm") == 0);
	vmm_remove(pid2,reg2);
	vmm_remove(pid1,reg1);
	checkMemoryAfter(true);

	checkMemoryBefore(true);
	thread_reserveFrames(2);
	test_assertTrue(vmm_add(pid1,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM,&reg1) == 0);
	test_assertTrue(vmm_add(pid2,NULL,0,PAGE_SIZE,PAGE_SIZE,REG_SHM,&reg2) == 0);
	thread_discardFrames();
	test_assertTrue(shm_create(pid,"myshm",6) >= 0);
	test_assertTrue(shm_join(pid1,"myshm") >= 0);
	test_assertTrue(shm_join(pid2,"myshm") >= 0);
	shm_remProc(pid2);
	shm_remProc(pid);
	vmm_remove(pid2,reg2);
	vmm_remove(pid1,reg1);
	checkMemoryAfter(true);

	proc_kill(pid1);
	proc_kill(pid2);

	test_caseSucceeded();
}
