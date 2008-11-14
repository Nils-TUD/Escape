/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/proc.h"
#include "../h/video.h"
#include "../h/paging.h"
#include "../h/mm.h"
#include "test.h"
#include "proc.h"

/* forward declarations */
static void test_proc(void);
static void test_proc_chgSize(s32 change,chgArea area);

/* our test-module */
tTestModule tModProc = {
	"Process-Management",
	&test_proc
};

s32 oldFF, newFF, oldPC, newPC;

/**
 * Stores the current page-count and free frames and starts a test-case
 */
static void test_init(cstring fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	test_caseStartv(fmt,ap);
	va_end(ap);

	oldPC = paging_getPageCount();
	oldFF = mm_getNumberOfFreeFrames(MM_DEF);
}

/**
 * Checks wether the page-count and free-frames are still the same and finishes the test-case
 */
static void test_check(void) {
	newPC = paging_getPageCount();
	newFF = mm_getNumberOfFreeFrames(MM_DEF);
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
	s32 changes[] = {0,1,10,1024,1025,2048,2047,2049,mm_getNumberOfFreeFrames(MM_DEF) + 1};
	chgArea areas[] = {CHG_DATA,CHG_STACK};

	/* test process clone & destroy */
	test_init("Cloning and destroying processes");
	for(x = 0; x < 5; x++) {
		u32 newPid = proc_getFreePid();
		tprintf("Cloning process to pid=%d\n",newPid);
		proc_clone(newPid);
		tprintf("Destroying process\n",newPid);
		proc_destroy(procs + newPid);
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

			proc_changeSize(changes[x],areas[y]);
			for(z = 0; z < ARRAY_SIZE(changes) - 1; z++) {
				tprintf("Changing by %d and rewinding\n",changes[z]);
				proc_changeSize(changes[z],areas[y]);
				proc_changeSize(-changes[z],areas[y]);
			}
			proc_changeSize(-changes[x],areas[y]);

			test_check();
		}
	}

	/* allocate all, free all */
	for(y = 0; y < ARRAY_SIZE(areas); y++) {
		test_init("Change area %d (allocate all, free all)",areas[y]);

		for(x = 0; x < ARRAY_SIZE(changes) - 1; x++) {
			tprintf("Allocating %d pages\n",changes[x]);
			proc_changeSize(changes[x],areas[y]);
		}
		for(x = 0; x < ARRAY_SIZE(changes) - 1; x++) {
			tprintf("Freeing %d pages\n",changes[x]);
			proc_changeSize(-changes[x],areas[y]);
		}

		test_check();
	}
}
