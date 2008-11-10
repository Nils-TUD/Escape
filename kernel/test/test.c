/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/video.h"
#include "test.h"
#include <stdarg.h>

tTestModule *modules[MAX_TESTS];
u32 moduleCount = 0;
u32 testCase = 0;

u32 modsSucc = 0;
u32 modsFailed = 0;
u32 totalSucc = 0;
u32 totalFail = 0;
u32 succCount = 0;
u32 failCount = 0;

void test_noPrint(cstring fmt,...) {
	/* do nothing */
}
void test_vnoPrint(cstring fmt,va_list ap) {
	/* do nothing */
}

void test_caseStart(cstring fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	test_caseStartv(fmt,ap);
	va_end(ap);
}

void test_caseStartv(cstring fmt,va_list ap) {
	vid_printf("== Testcase %d : ",testCase++);
	vid_vprintf(fmt,ap);
	vid_printf(" ==\n");
}

void test_caseSucceded(void) {
	vid_printf("== >> %:02s ==\n\n","SUCCESS");
	totalSucc++;
	succCount++;
}

void test_caseFailed(cstring fmt,...) {
	va_list ap;
	vid_printf("== >> %:04s : ","FAILED");
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
	vid_printf(" ==\n\n");
	totalFail++;
	failCount++;
}

void test_register(tTestModule *mod) {
	modules[moduleCount++] = mod;
}

void test_start(void) {
	vid_printf("\n====== Starting test-procedure ======\n");

	u32 i;
	for(i = 0; i < moduleCount; i++) {
		vid_printf("---- Starting with module %d : \"%s\" ----\n\n",i,modules[i]->name);

		testCase = 1;
		succCount = 0;
		failCount = 0;
		modules[i]->start();

		if(!failCount)
			modsSucc++;
		else
			modsFailed++;

		vid_printf("---- Module \"%s\" finished. Summary: ----\n",modules[i]->name);
		vid_printf("-- %:02d testcases successfull --\n",succCount);
		vid_printf("-- %:04d testcases failed --\n",failCount);
		vid_printf("----------------------------------\n\n");
	}

	vid_printf("====== All modules done ======\n");
	vid_printf("== %:02d modules successfull ==\n",modsSucc);
	vid_printf("== %:04d modules failed ==\n",modsFailed);
	vid_printf("== %:02d testcases successfull ==\n",totalSucc);
	vid_printf("== %:04d testcases failed ==\n",totalFail);
	vid_printf("============================\n");
}
