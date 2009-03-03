/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifdef IN_KERNEL
#	include "../../kernel/h/common.h"
#	include <video.h>

#	define testPrintf	vid_printf
#	define testvPrintf	vid_vprintf
#else
#	include "../../libc/h/common.h"
#	include "../../libc/h/debug.h"

#	define testPrintf	debugf
#	define testvPrintf	vdebugf
#endif

#include "../h/test.h"
#include <stdarg.h>

static sTestModule *modules[MAX_TESTS];
static u32 moduleCount = 0;
static u32 testCase = 0;

static u32 modsSucc = 0;
static u32 modsFailed = 0;
static u32 totalSucc = 0;
static u32 totalFail = 0;
static u32 succCount = 0;
static u32 failCount = 0;

static u32 assertCount = 0;

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
	testPrintf("== Testcase %d : ",testCase++);
	testvPrintf(fmt,ap);
	testPrintf(" ==\n");
	assertCount = 0;
}

void test_caseSucceded(void) {
	testPrintf("== >> \e[32m%s\e[0m ==\n\n","SUCCESS");
	totalSucc++;
	succCount++;
}

bool test_assertTrue(bool received) {
	assertCount++;
	if(!received) {
		test_caseFailed("Assert %d: Received false, expected true",assertCount);
		return false;
	}
	return true;
}

bool test_assertFalse(bool received) {
	assertCount++;
	if(received) {
		test_caseFailed("Assert %d: Received true, expected false",assertCount);
		return false;
	}
	return true;
}

bool test_assertPtr(void *received,void *expected) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d: Pointers are not equal: 0x%x != 0x%x",assertCount,
				expected,received);
		return false;
	}
	return true;
}

bool test_assertInt(s32 received,s32 expected) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d: Integers are not equal: %d != %d",assertCount,
				expected,received);
		return false;
	}
	return true;
}

bool test_assertUInt(u32 received,u32 expected) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d: Integers are not equal: 0x%x != 0x%x",assertCount,
				expected,received);
		return false;
	}
	return true;
}

bool test_assertStr(string received,string expected) {
	string s1 = expected;
	string s2 = received;
	assertCount++;
	while(*s1 && *s2) {
		if(*s1 != *s2) {
			test_caseFailed("Assert %d: Strings are not equal: '%s' != '%s'",assertCount,
					expected,received);
			return false;
		}
		s1++;
		s2++;
	}
	if(*s1 != *s2) {
		test_caseFailed("Assert %d: Strings are not equal: '%s' != '%s'",assertCount,
				expected,received);
		return false;
	}
	testPrintf("ASSERT %d succeded\n",assertCount);
	return true;
}

void test_caseFailed(cstring fmt,...) {
	va_list ap;
	testPrintf("== >> \e[31m%s\e[0m : ","FAILED");
	va_start(ap,fmt);
	testvPrintf(fmt,ap);
	va_end(ap);
	testPrintf(" ==\n\n");
	totalFail++;
	failCount++;
}

void test_register(sTestModule *mod) {
	modules[moduleCount++] = mod;
}

void test_start(void) {
	testPrintf("\n====== Starting test-procedure ======\n");

	u32 i;
	for(i = 0; i < moduleCount; i++) {
		testPrintf("---- Starting with module %d : \"%s\" ----\n\n",i,modules[i]->name);

		testCase = 1;
		succCount = 0;
		failCount = 0;
		modules[i]->start();

		if(!failCount)
			modsSucc++;
		else
			modsFailed++;

		testPrintf("---- Module \"%s\" finished. Summary: ----\n",modules[i]->name);
		testPrintf("-- \e[32m%d\e[0m testcases successfull --\n",succCount);
		testPrintf("-- \e[31m%d\e[0m testcases failed --\n",failCount);
		testPrintf("----------------------------------\n\n");
	}

	testPrintf("====== All modules done ======\n");
	testPrintf("== \e[32m%d\e[0m modules successfull ==\n",modsSucc);
	testPrintf("== \e[31m%d\e[0m modules failed ==\n",modsFailed);
	testPrintf("== \e[32m%d\e[0m testcases successfull ==\n",totalSucc);
	testPrintf("== \e[31m%d\e[0m testcases failed ==\n",totalFail);
	testPrintf("============================\n");
}
