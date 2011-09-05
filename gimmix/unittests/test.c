/**
 * $Id: test.c 227 2011-06-11 18:40:58Z nasmussen $
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "mmix/io.h"
#include "mmix/mem.h"
#include "exception.h"
#include "test.h"

// one test-suite
typedef struct sTestSuite {
	const char *name;
	const sTestCase *cases;
	struct sTestSuite *next;
} sTestSuite;

static void test_updateLast(const char *func,int line);
static void test_caseFailed(const char *func,int line,const char *fmt,...);

static sTestSuite *firstSuite;
static sTestSuite *lastSuite;
static int suiteCount = 0;
static int failCount = 0;
static const char *lastFunc = NULL;
static int lastLine = -1;

// we're using global variables here, because longjmp might clobber them otherwise (in test_start)
static int totalSucc;
static int totalFailed;
static int suitesSucc;
static sTestSuite *suite;
static int suiteIndex;
static int casesSucc = 0;
static int casesFailed = 0;

bool test_doAssertTrue(bool received,const char *func,int line) {
	if(!received) {
		test_caseFailed(func,line,"Received false, expected true");
		return false;
	}
	test_updateLast(func,line);
	return true;
}

bool test_doAssertFalse(bool received,const char *func,int line) {
	if(received) {
		test_caseFailed(func,line,"Received true, expected false");
		return false;
	}
	test_updateLast(func,line);
	return true;
}

bool test_doAssertPtr(const void *received,const void *expected,const char *func,int line) {
	if(expected != received) {
		test_caseFailed(func,line,"Pointers are not equal: Expected %p, got %p",expected,received);
		return false;
	}
	test_updateLast(func,line);
	return true;
}

bool test_doAssertInt(int received,int expected,const char *func,int line) {
	if(expected != received) {
		test_caseFailed(func,line,"Integers are not equal: Expected %d, got %d",expected,received);
		return false;
	}
	test_updateLast(func,line);
	return true;
}

bool test_doAssertUInt(unsigned int received,unsigned int expected,const char *func,int line) {
	if(expected != received) {
		test_caseFailed(func,line,"Integers are not equal: Expected %#x, got %#x",expected,received);
		return false;
	}
	test_updateLast(func,line);
	return true;
}

bool test_doAssertByte(byte received,byte expected,const char *func,int line) {
	if(expected != received) {
		test_caseFailed(func,line,"Bytes are not equal: Expected %#Bx, got %#Bx",expected,received);
		return false;
	}
	test_updateLast(func,line);
	return true;
}

bool test_doAssertWyde(wyde received,wyde expected,const char *func,int line) {
	if(expected != received) {
		test_caseFailed(func,line,"Wydes are not equal: Expected %#Wx, got %#Wx",expected,received);
		return false;
	}
	test_updateLast(func,line);
	return true;
}

bool test_doAssertTetra(tetra received,tetra expected,const char *func,int line) {
	if(expected != received) {
		test_caseFailed(func,line,"Tetras are not equal: Expected %#Tx, got %#Tx",expected,received);
		return false;
	}
	test_updateLast(func,line);
	return true;
}

bool test_doAssertOcta(octa received,octa expected,const char *func,int line) {
	if(expected != received) {
		test_caseFailed(func,line,"Octas are not equal: Expected %#Ox, got %#Ox",expected,received);
		return false;
	}
	test_updateLast(func,line);
	return true;
}

bool test_doAssertStr(const char *received,const char *expected,const char *func,int line) {
	if(expected == NULL && received == NULL)
		return true;
	if(expected == NULL || received == NULL)
		return false;
	if(strcmp(received,expected) != 0) {
		test_caseFailed(func,line,"Strings are not equal: Expected '%s', got '%s'",expected,received);
		return false;
	}
	test_updateLast(func,line);
	return true;
}

void test_register(const char *name,const sTestCase *cases) {
	sTestSuite *s = (sTestSuite*)mem_alloc(sizeof(sTestSuite));
	s->name = name;
	s->cases = cases;
	if(lastSuite)
		lastSuite->next = s;
	lastSuite = s;
	s->next = NULL;
	if(firstSuite == NULL)
		firstSuite = s;
	suiteCount++;
}

void test_start(void) {
	totalSucc = 0;
	totalFailed = 0;
	suitesSucc = 0;
	suite = firstSuite;
	for(suiteIndex = 0; suiteIndex < suiteCount; suiteIndex++) {
		mprintf("\033[1mTestsuite \"%s\"...\033[0m\n",suite->name);

		failCount = 0;
		casesSucc = 0;
		casesFailed = 0;
		for(size_t j = 0; suite->cases[j].name != NULL ; j++) {
			mprintf("\tTestcase \"%s\"...\n",suite->cases[j].name);
			failCount = 0;
			lastFunc = NULL;
			lastLine = -1;
			jmp_buf env;
			int ex = setjmp(env);
			if(ex != EX_NONE) {
				if(lastLine != -1)
					mprintf("\tLast succeeded assert in %s, line %d\n",lastFunc,lastLine);
				mprintf("\tThrew exception: %s\n",ex_toString(ex,ex_getBits()));
				failCount++;
			}
			else {
				ex_push(&env);
				suite->cases[j].func();
			}
			ex_pop();

			if(failCount == 0) {
				mprintf("\t\033[0;32mSUCCEEDED!\033[0m\n");
				casesSucc++;
			}
			else {
				mprintf("\t\033[0;31mFAILED!\033[0m\n");
				casesFailed++;
			}
		}
		totalSucc += casesSucc;
		totalFailed += casesFailed;
		if(casesFailed == 0)
			suitesSucc++;

		if(casesFailed == 0)
			mprintf("\033[0;32m%d\033[0m of %d testcases successfull\n",casesSucc,casesSucc);
		else
			mprintf("\033[0;31m%d\033[0m of %d testcases successfull\n",
					casesSucc,casesSucc + casesFailed);
		mprintf("\n");
		suite = suite->next;
	}

	mprintf("\033[1mAll tests done:\033[0m\n");
	if(suitesSucc == suiteCount)
		mprintf("\033[0;32m%d\033[0m of %d testsuites successfull\n",suitesSucc,suiteCount);
	else
		mprintf("\033[0;31m%d\033[0m of %d testsuites successfull\n",suitesSucc,suiteCount);
	if(totalFailed == 0)
		mprintf("\033[0;32m%d\033[0m of %d testcases successfull\n",totalSucc,totalSucc);
	else
		mprintf("\033[0;31m%d\033[0m of %d testcases successfull\n",
				totalSucc,totalSucc + totalFailed);
}

void test_cleanup(void) {
	sTestSuite *s = firstSuite;
	for(int i = 0; i < suiteCount; i++) {
		sTestSuite *next = s->next;
		mem_free(s);
		s = next;
	}
}

static void test_updateLast(const char *func,int line) {
	lastFunc = func;
	lastLine = line;
}

static void test_caseFailed(const char *func,int line,const char *fmt,...) {
	va_list ap;
	mprintf("\tAssert in %s line %d failed: ",func,line);
	va_start(ap,fmt);
	mvprintf(fmt,ap);
	va_end(ap);
	mprintf("\n");
	failCount++;
}
