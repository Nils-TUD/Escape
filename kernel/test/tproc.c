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

#include <common.h>
#include <task/proc.h>
#include <mem/paging.h>
#include <mem/pmem.h>
#include <video.h>
#include <test.h>
#include "tproc.h"

/* forward declarations */
static void test_proc(void);

/* our test-module */
sTestModule tModProc = {
	"Process-Management",
	&test_proc
};

s32 oldFF, newFF, oldPC, newPC;

/**
 * Stores the current page-count and free frames and starts a test-case
 */
static void test_init(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	test_caseStartv(fmt,ap);
	va_end(ap);

	oldPC = paging_dbg_getPageCount();
	oldFF = mm_getFreeFrmCount(MM_DEF);
}

/**
 * Checks wether the page-count and free-frames are still the same and finishes the test-case
 */
static void test_check(void) {
	newPC = paging_dbg_getPageCount();
	newFF = mm_getFreeFrmCount(MM_DEF);
	if(oldFF != newFF || oldPC != newPC) {
		test_caseFailed("oldPC=%d, oldFF=%d, newPC=%d, newFF=%d",oldPC,oldFF,newPC,newFF);
	}
	else {
		test_caseSucceded();
	}
}

static void test_proc(void) {
	u32 x,y,z;
	bool res;
	s32 changes[] = {/*0,1,10,1024,1025,*/2048,2047,2049,0};
	eChgArea areas[] = {CHG_DATA,CHG_STACK};
	changes[ARRAY_SIZE(changes) - 1] = mm_getFreeFrmCount(MM_DEF) + 1;

	/* test process clone & destroy */
	test_init("Cloning and destroying processes");
	for(x = 0; x < 5; x++) {
		tPid newPid = proc_getFreePid();
		tprintf("Cloning process to pid=%d\n",newPid);
		test_assertTrue(proc_clone(newPid) >= 0);
		tprintf("Destroying process\n",newPid);
		proc_destroy(proc_getByPid(newPid));
	}
	test_check();

	/* allocate one, free one */
	for(y = 0; y < ARRAY_SIZE(areas); y++) {
		for(x = 0; x < ARRAY_SIZE(changes); x++) {
			test_init("Change area %d by %d (allocate, free)",areas[y],changes[x]);
			res = true;

			res = res && (changes[x] > oldFF ? !proc_changeSize(changes[x],areas[y]) :
				proc_changeSize(changes[x],areas[y]));
			if(changes[x] <= oldFF) {
				res = res && proc_changeSize(-changes[x],areas[y]);
			}
			else {
				vid_printf("Not enough mem or invalid segment sizes!\n");
			}

			test_check();
		}
	}

	/* allocate one, allocate one other, free other, free first */
	for(y = 0; y < ARRAY_SIZE(areas); y++) {
		for(x = 0; x < ARRAY_SIZE(changes) - 1; x++) {
			test_init("Change area %d by %d (allocate, allocate, free, free)",areas[y],changes[x]);

			test_assertTrue(proc_changeSize(changes[x],areas[y]));
			for(z = 0; z < ARRAY_SIZE(changes) - 1; z++) {
				tprintf("Changing by %d and rewinding\n",changes[z]);
				test_assertTrue(proc_changeSize(changes[z],areas[y]));
				test_assertTrue(proc_changeSize(-changes[z],areas[y]));
			}
			test_assertTrue(proc_changeSize(-changes[x],areas[y]));

			test_check();
		}
	}

	/* allocate all, free all */
	for(y = 0; y < ARRAY_SIZE(areas); y++) {
		test_init("Change area %d (allocate all, free all)",areas[y]);

		for(x = 0; x < ARRAY_SIZE(changes) - 1; x++) {
			tprintf("Allocating %d pages\n",changes[x]);
			test_assertTrue(proc_changeSize(changes[x],areas[y]));
		}
		for(x = 0; x < ARRAY_SIZE(changes) - 1; x++) {
			tprintf("Freeing %d pages\n",changes[x]);
			test_assertTrue(proc_changeSize(-changes[x],areas[y]));
		}

		test_check();
	}
}
