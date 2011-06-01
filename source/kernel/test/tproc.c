/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/video.h>
#include <esc/test.h>
#include "tproc.h"

/* forward declarations */
static void test_proc(void);

/* our test-module */
sTestModule tModProc = {
	"Process-Management",
	&test_proc
};

static size_t oldFF, newFF;

/**
 * Stores the current page-count and free frames and starts a test-case
 */
static void test_init(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	test_caseStartv(fmt,ap);
	va_end(ap);

	oldFF = pmem_getFreeFrames(MM_DEF);
}

/**
 * Checks whether the page-count and free-frames are still the same and finishes the test-case
 */
static void test_check(void) {
	newFF = pmem_getFreeFrames(MM_DEF);
	if(oldFF != newFF) {
		test_caseFailed("oldFF=%d, newFF=%d",oldFF,newFF);
	}
	else {
		test_caseSucceeded();
	}
}

static void test_proc(void) {
	size_t x;

	/* test process clone & destroy */
	test_init("Cloning and destroying processes");
	for(x = 0; x < 5; x++) {
		tPid newPid = proc_getFreePid();
		tprintf("Cloning process to pid=%d\n",newPid);
		test_assertTrue(proc_clone(newPid,0) >= 0);
		tprintf("Destroying process\n",newPid);
		/* both are necessary */
		proc_terminate(proc_getByPid(newPid),0,0);
		proc_kill(proc_getByPid(newPid));
	}
	test_check();
}
