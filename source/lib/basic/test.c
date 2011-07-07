/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <esc/test.h>
#include <stdarg.h>
#include <assert.h>

#ifdef IN_KERNEL
#	include <sys/video.h>

#	define testPrintf	vid_printf
#	define testvPrintf	vid_vprintf
#else
#	include <esc/proc.h>
#	include <stdio.h>

#	define testPrintf	printf
#	define testvPrintf	vprintf
#endif

static sTestModule *modules[MAX_TESTS];
static size_t moduleCount = 0;
static size_t testCase = 0;

static size_t modsSucc = 0;
static size_t modsFailed = 0;
static size_t totalSucc = 0;
static size_t totalFail = 0;
static size_t succCount = 0;
static size_t failCount = 0;

static size_t assertCount = 0;

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

void test_caseSucceeded(void) {
	testPrintf("== >> \033[co;2]%s\033[co] ==\n\n","SUCCESS");
	totalSucc++;
	succCount++;
}

bool test_doAssertTrue(bool received,const char *func,int line) {
	assertCount++;
	if(!received) {
		test_caseFailed("Assert %d in %s line %d: Received false, expected true",assertCount,func,line);
		return false;
	}
	return true;
}

bool test_doAssertFalse(bool received,const char *func,int line) {
	assertCount++;
	if(received) {
		test_caseFailed("Assert %d in %s line %d: Received true, expected false",assertCount,func,line);
		return false;
	}
	return true;
}

bool test_doAssertInt(int received,int expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: Expected %d, got %d",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertUInt(uint received,uint expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: Expected 0x%x, got 0x%x",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertLInt(long received,long expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: Expected %ld, got %ld",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertULInt(ulong received,ulong expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: "
				"Expected 0x%lx, got 0x%lx",assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertLLInt(llong received,llong expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: Expected %Ld, got %Ld",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertULLInt(ullong received,ullong expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: "
				"Expected 0x%Lx, got 0x%Lx",assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertPtr(const void *received,const void *expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Pointers are not equal: Expected %p, got %p",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertUIntPtr(uintptr_t received,uintptr_t expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: Expected 0x%Px, got 0x%Px",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertSize(size_t received,size_t expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: Expected 0x%zx, got 0x%zx",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertSSize(ssize_t received,ssize_t expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: Expected 0x%zx, got 0x%zx",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertOff(off_t received,off_t expected,const char *func,int line) {
	assertCount++;
	if(expected != received) {
		test_caseFailed("Assert %d in %s line %d: Integers are not equal: Expected 0x%Ox, got 0x%Ox",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

bool test_doAssertStr(const char *received,const char *expected,const char *func,int line) {
	char *s1 = (char*)expected;
	char *s2 = (char*)received;
	assertCount++;
	if(s1 == NULL && s2 == NULL)
		return true;
	if(s1 == NULL || s2 == NULL)
		return false;
	while(*s1 && *s2) {
		if(*s1 != *s2) {
			test_caseFailed("Assert %d in %s line %d: Strings are not equal: Expected '%s', got '%s'",
					assertCount,func,line,expected,received);
			return false;
		}
		s1++;
		s2++;
	}
	if(*s1 != *s2) {
		test_caseFailed("Assert %d in %s line %d: Strings are not equal: Expected '%s', got '%s'",
				assertCount,func,line,expected,received);
		return false;
	}
	return true;
}

void test_caseFailed(const char *fmt,...) {
	va_list ap;
	testPrintf("== >> \033[co;4]%s\033[co] : ","FAILED");
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
	size_t i;
	testPrintf("\n====== Starting test-procedure ======\n");

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
		testPrintf("-- \033[co;2]%d\033[co] testcases successfull --\n",succCount);
		testPrintf("-- \033[co;4]%d\033[co] testcases failed --\n",failCount);
		testPrintf("----------------------------------\n\n");
	}

	testPrintf("====== All modules done ======\n");
	testPrintf("== \033[co;2]%d\033[co] modules successfull ==\n",modsSucc);
	testPrintf("== \033[co;4]%d\033[co] modules failed ==\n",modsFailed);
	testPrintf("== \033[co;2]%d\033[co] testcases successfull ==\n",totalSucc);
	testPrintf("== \033[co;4]%d\033[co] testcases failed ==\n",totalFail);
	testPrintf("============================\n");
}
