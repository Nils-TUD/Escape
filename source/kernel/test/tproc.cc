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
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/mem/paging.h>
#include <sys/mem/physmem.h>
#include <sys/video.h>
#include <esc/test.h>
#include <assert.h>
#include "testutils.h"
#include "tproc.h"

/* forward declarations */
static void test_proc(void);
static void test_proc_clone(void);
static void test_thread(void);

/* our test-module */
sTestModule tModProc = {
	"Process-Management",
	&test_proc
};

static void test_proc(void) {
	test_proc_clone();
	test_thread();
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

static void test_proc_clone(void) {
	size_t x;

	/* test process clone & destroy */
	test_init("Cloning and destroying processes");
	for(x = 0; x < 5; x++) {
		pid_t pid;
		tprintf("Cloning process\n");
		pid = Proc::clone(0);
		test_assertTrue(pid > 0);
		tprintf("Destroying process\n");
		/* first terminate it, then free resources and finally remove the process */
		assert(Proc::getByPid(pid)->getMainThread()->beginTerm());
		Proc::destroy(pid);
		Proc::kill(pid);
	}
	checkMemoryAfter(false);
	test_caseSucceeded();
}

static int threadcnt = 0;

static void thread_test(void) {
	tprintf("thread %d is running...\n",Thread::getRunning()->getTid());
	threadcnt++;

	Proc::exit(0);
	Thread::switchAway();
}

static void test_thread(void) {
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
