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

#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <sys/test.h>
#include <task/proc.h>
#include <task/thread.h>
#include <assert.h>
#include <common.h>
#include <video.h>

#include "testutils.h"

/* forward declarations */
static void test_proc();
static void test_proc_clone();
static void test_thread();

/* our test-module */
sTestModule tModProc = {
	"Process-Management",
	&test_proc
};

static void test_proc() {
	/* doesn't work on mmix since we would have to leave the kernel and enter it again in order to
	 * get a new kernel-stack */
#ifndef __mmix__
	test_proc_clone();
	test_thread();
#endif
}

/**
 * Stores the current page-count and free frames and starts a test-case
 */
static void test_init(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	test_caseStartv(fmt,ap);
	va_end(ap);
	checkMemoryBefore(false);
}

static void test_proc_clone() {
	/* test process clone & destroy */
	test_init("Cloning and destroying processes");
	for(size_t x = 0; x < 5; x++) {
		pid_t pid;
		tprintf("Cloning process\n");
		pid = Proc::clone(0);
		if(pid == 0) {
			tprintf("Destroying myself\n");
			Proc::terminate(0);
			A_UNREACHED;
		}
		else {
			test_assertTrue(pid > 0);
			tprintf("Waiting for child\n");
			Proc::waitChild(NULL,-1,0);
			tprintf("The process should be dead now\n");
			test_assertTrue(Proc::getByPid(pid) == NULL);
		}
	}
	checkMemoryAfter(false);
	test_caseSucceeded();
}

static int threadcnt = 0;

static void thread_test() {
	tprintf("thread %d is running...\n",Thread::getRunning()->getTid());
	threadcnt++;

	Proc::terminateThread(0);
}

static void test_thread() {
	test_init("Starting threads and joining them");

	int tid = Proc::startThread((uintptr_t)&thread_test,0,NULL);
	test_assertTrue(tid >= 0);
	int tid2 = Proc::startThread((uintptr_t)&thread_test,0,NULL);
	test_assertTrue(tid2 >= 0);
	Proc::join(tid);
	Proc::join(tid2);
	test_assertInt(threadcnt,2);
	tprintf("threads finished\n");

	test_caseSucceeded();
}
