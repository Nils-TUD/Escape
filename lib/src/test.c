/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <types.h>
#include <test.h>
#include <stdarg.h>
#include <assert.h>

#ifdef IN_KERNEL
#	include <video.h>

#	define testPrintf	vid_printf
#	define testvPrintf	vid_vprintf
#else
#	include <esc/fileio.h>
#	include <esc/proc.h>

#	define testPrintf	printf
#	define testvPrintf	vprintf
#endif

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

void test_noPrint(const char *fmt,...) {
	/* do nothing */
	UNUSED(fmt);
}
void test_vnoPrint(const char *fmt,va_list ap) {
	/* do nothing */
	UNUSED(fmt);
	UNUSED(ap);
}

void test_caseStart(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	test_caseStartv(fmt,ap);
	va_end(ap);
}

void test_caseStartv(const char *fmt,va_list ap) {
	testPrintf("== Testcase %d : ",testCase++);
	testvPrintf(fmt,ap);
	testPrintf(" ==\n");
	assertCount = 0;
}

void test_caseSucceded(void) {
	testPrintf("== >> \033f\x2%s\033r\x0 ==\n\n","SUCCESS");
	totalSucc++;
	succCount++;
}

bool test_assertTrue(bool received) {
	assertCount++;
	if(!received) {
		test_caseFailed("Assert %d: Received false, expected true",assertCount);
		return false;
	}
	testPrintf("vassert %d succeded\n",assertCount);
	return true;
}

bool test_assertFalse(bool received) {
	assertCount++;
	if(received) {
		test_caseFailed("Assert %d: Received true, expected false",assertCount);
		return false;
	}
	testPrintf("vassert %d succeded\n",assertCount);
	return true;
}

bool test_assertPtr(const void *received,const void *expected) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d: Pointers are not equal: Expected 0x%x, got 0x%x",assertCount,
				expected,received);
		return false;
	}
	testPrintf("vassert %d succeded\n",assertCount);
	return true;
}

bool test_assertInt(s32 received,s32 expected) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d: Integers are not equal: Expected %d, got %d",assertCount,
				expected,received);
		return false;
	}
	testPrintf("vassert %d succeded\n",assertCount);
	return true;
}

bool test_assertUInt(u32 received,u32 expected) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d: Integers are not equal: Expected 0x%x, got 0x%x",assertCount,
				expected,received);
		return false;
	}
	testPrintf("vassert %d succeded\n",assertCount);
	return true;
}

bool test_assertStr(const char *received,const char *expected) {
	char *s1 = (char*)expected;
	char *s2 = (char*)received;
	assertCount++;
	if(s1 == NULL && s2 == NULL)
		return true;
	if(s1 == NULL || s2 == NULL)
		return false;
	while(*s1 && *s2) {
		if(*s1 != *s2) {
			test_caseFailed("Assert %d: Strings are not equal: Expected '%s', got '%s'",assertCount,
					expected,received);
			return false;
		}
		s1++;
		s2++;
	}
	if(*s1 != *s2) {
		test_caseFailed("Assert %d: Strings are not equal: Expected '%s', got '%s'",assertCount,
				expected,received);
		return false;
	}
	testPrintf("vassert %d succeded\n",assertCount);
	return true;
}

void test_caseFailed(const char *fmt,...) {
	va_list ap;
	testPrintf("== >> \033f\x4%s\033r\x0 : ","FAILED");
	va_start(ap,fmt);
	testvPrintf(fmt,ap);
	va_end(ap);
	testPrintf(" ==\n\n");
	totalFail++;
	failCount++;
}

void test_register(sTestModule *mod) {
	vassert(moduleCount < MAX_TESTS,"Max. tests reached");
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
		testPrintf("-- \033f\x2%d\033r\x0 testcases successfull --\n",succCount);
		testPrintf("-- \033f\x4%d\033r\x0 testcases failed --\n",failCount);
		testPrintf("----------------------------------\n\n");
	}

	testPrintf("====== All modules done ======\n");
	testPrintf("== \033f\x2%d\033r\x0 modules successfull ==\n",modsSucc);
	testPrintf("== \033f\x4%d\033r\x0 modules failed ==\n",modsFailed);
	testPrintf("== \033f\x2%d\033r\x0 testcases successfull ==\n",totalSucc);
	testPrintf("== \033f\x4%d\033r\x0 testcases failed ==\n",totalFail);
	testPrintf("============================\n");
}
