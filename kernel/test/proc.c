/**
 * @version		$Id: test.c 37 2008-11-09 16:58:20Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/proc.h"
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

static void test_proc(void) {
	/*while(1) {
		vid_printf("free frames (MM_DEF) = %d\n",mm_getNumberOfFreeFrames(MM_DEF));
		if(!proc_clone(procs + 1)) {
			panic("proc_clone: Not enough memory!!!");
		}
	}*/

	u32 x,y;
	s32 changes[] = {0,1,10,1024,1025,2048,2047,2049,mm_getNumberOfFreeFrames(MM_DEF) + 1};
	chgArea areas[] = {CHG_DATA,CHG_STACK};

	for(y = 0; y < ARRAY_SIZE(areas); y++) {
		for(x = 0; x < ARRAY_SIZE(changes); x++) {
			test_proc_chgSize(changes[x],areas[y]);
		}
	}
}

static void test_proc_chgSize(s32 change,chgArea area) {
	bool res = true;
	s32 oldFF, newFF, oldPC, newPC;

	test_caseStart("Change area %d by %d",area,change);

	oldPC = paging_getPageCount();
	oldFF = mm_getNumberOfFreeFrames(MM_DEF);

	res = res && (change > oldFF ? !proc_changeSize(change,area) : proc_changeSize(change,area));
	if(change <= oldFF) {
		res = res && proc_changeSize(-change,area);
	}
	else {
		vid_printf("Not enough mem or invalid segment sizes!\n");
	}

	newPC = paging_getPageCount();
	newFF = mm_getNumberOfFreeFrames(MM_DEF);

	if(!res || oldFF != newFF || oldPC != newPC) {
		test_caseFailed("oldPC=%d, oldFF=%d, newPC=%d, newFF=%d",oldPC,oldFF,newPC,newFF);
	}
	else {
		test_caseSucceded();
	}
}
