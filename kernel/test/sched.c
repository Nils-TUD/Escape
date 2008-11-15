/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/sched.h"

#include "test.h"
#include "sched.h"

/* forward declarations */
static void test_sched(void);

/* our test-module */
tTestModule tModSched = {
	"Scheduling",
	&test_sched
};

static void test_sched(void) {
	u32 i;
	bool res = true;
	/* not relevant here */
	tProc *p = (tProc*)0x1000, *pp;

	test_caseStart("Enqueuing - dequeuing");

	/* enqueue */
	for(i = 0; i < PROC_COUNT - 1; i++) {
		sched_enqueueReady(p);
		p++;
	}

	/* dequeue */
	pp = (tProc*)0x1000;
	while((p = sched_dequeueReady()) != NULL) {
		if(p != pp) {
			res = false;
			break;
		}
		pp++;
	}

	if(res)
		test_caseSucceded();
	else
		test_caseFailed("");
}
