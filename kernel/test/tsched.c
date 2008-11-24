/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/sched.h"

#include "test.h"
#include "tsched.h"

/* forward declarations */
static void test_sched(void);

/* our test-module */
sTestModule tModSched = {
	"Scheduling",
	&test_sched
};

static void test_sched(void) {
	sProc *x = (sProc*)0x1000;
	sProc *rand[5] = {x + 1,x,x + 4,x + 2,x + 3};
	s32 i;
	bool res = true;
	/* not relevant here */
	sProc *p = (sProc*)x, *pp;

	test_caseStart("Enqueuing - dequeuing");
	/* enqueue */
	for(i = 0; i < PROC_COUNT - 1; i++) {
		sched_enqueueReady(p++);
	}
	/* dequeue */
	pp = (sProc*)x;
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
		test_caseFailed("Got process 0x%x, expected 0x%x",p,pp);

	/* second test */
	res = true;
	test_caseStart("Dequeue specific processes (opposite order)");
	/* enqueue some processes */
	p = (sProc*)x;
	for(i = 0; i < 5; i++)
		sched_enqueueReady(p++);
	/* dequeue */
	p--;
	for(i = 4; i >= 0; i--) {
		res = res && sched_dequeueProc(p);
		res = res && !sched_dequeueProc(p);
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
		sched_enqueueReady(p++);
	/* dequeue */
	for(i = 0; i < 5; i++) {
		res = res && sched_dequeueProc(rand[i]);
		res = res && !sched_dequeueProc(rand[i]);
	}
	if(res)
		test_caseSucceded();
	else
		test_caseFailed("");
}
