/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <sched.h>

#include <test.h>
#include "tsched.h"

/* forward declarations */
static void test_sched(void);

/* our test-module */
sTestModule tModSched = {
	"Scheduling",
	&test_sched
};

static void test_sched(void) {
	sProc *x = (sProc*)proc_getByPid(proc_getFreePid());
	sProc *rand[5] = {x + 1,x,x + 4,x + 2,x + 3};
	s32 i;
	bool res = true;
	/* not relevant here */
	sProc *p = (sProc*)x, *pp;

	test_caseStart("Enqueuing - dequeuing");
	/* enqueue */
	for(i = 0; i < PROC_COUNT - 1; i++) {
		sched_setReady(p++);
	}
	/* dequeue */
	pp = (sProc*)x;
	while((p = sched_dequeueReady()) != NULL) {
		p->state = ST_UNUSED;
		if(p != pp) {
			res = false;
			break;
		}
		pp++;
	}
	if(res)
		test_caseSucceded();
	else
		test_caseFailed("Got process 0x%x, expected 0x%x",p,pp);

	/* second test */
	res = true;
	test_caseStart("Dequeue specific processes (opposite order)");
	/* enqueue some processes */
	p = (sProc*)x;
	for(i = 0; i < 5; i++)
		sched_setReady(p++);
	/* dequeue */
	p--;
	for(i = 4; i >= 0; i--) {
		res = res && sched_dequeueReadyProc(p);
		res = res && !sched_dequeueReadyProc(p);
		p->state = ST_UNUSED;
		p--;
	}
	if(res)
		test_caseSucceded();
	else
		test_caseFailed("");

	/* third test */
	res = true;
	test_caseStart("Dequeue specific processes (random order)");
	/* enqueue some processes */
	p = (sProc*)x;
	for(i = 0; i < 5; i++)
		sched_setReady(p++);
	/* dequeue */
	for(i = 0; i < 5; i++) {
		res = res && sched_dequeueReadyProc(rand[i]);
		res = res && !sched_dequeueReadyProc(rand[i]);
		rand[i]->state = ST_UNUSED;
	}
	if(res)
		test_caseSucceded();
	else
		test_caseFailed("");
}
