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

#pragma once

#include <esc/common.h>
#include <stdarg.h>

#define MAX_TESTS 100

typedef void (*fStart)(void);

/* one test-module */
typedef struct {
	const char *name;
	fStart start;
} sTestModule;

#ifdef TESTSQUIET
#	define tprintf test_noPrint
#	define tvprintf test_vnoPrint
#else
#	ifdef IN_KERNEL
#		define tprintf vid_printf
#		define tvprintf vid_vprintf
#	else
#		define tprintf printf
#		define tvprintf vprintf
#	endif
#endif

/* some macros for convenience and better error-reporting */
#define test_assertTrue(val) test_doAssertTrue((val),__FUNCTION__,__LINE__)
#define test_assertFalse(val) test_doAssertFalse((val),__FUNCTION__,__LINE__)
#define test_assertPtr(recv,exp) test_doAssertPtr((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertUIntPtr(recv,exp) test_doAssertUIntPtr((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertSize(recv,exp) test_doAssertSize((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertSSize(recv,exp) test_doAssertSSize((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertOff(recv,exp) test_doAssertOff((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertInt(recv,exp) test_doAssertInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertUInt(recv,exp) test_doAssertUInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertLInt(recv,exp) test_doAssertLInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertULInt(recv,exp) test_doAssertULInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertLLInt(recv,exp) test_doAssertLLInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertULLInt(recv,exp) test_doAssertULLInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertStr(recv,exp) test_doAssertStr((recv),(exp),__FUNCTION__,__LINE__)
#ifndef IN_KERNEL
#	define test_assertFloat(recv,exp) test_doAssertFloat((recv),(exp),__FUNCTION__,__LINE__)
#	define test_assertDouble(recv,exp) test_doAssertDouble((recv),(exp),__FUNCTION__,__LINE__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The "do-nothing" printf-function
 */
void test_noPrint(const char *fmt,...);
/**
 * The "do-nothing" vprintf-function
 */
void test_vnoPrint(const char *fmt,va_list ap);

/**
 * Starts a test-case with given name. This ends with a call of test_caseSucceeded() or
 * test_caseFailed().
 *
 * @param fmt the format of the string to print (see vid_printf)
 */
void test_caseStart(const char *fmt,...);

/**
 * Starts a test-case with given name. This ends with a call of test_caseSucceeded() or
 * test_caseFailed().
 *
 * @param fmt the format of the string to print (see vid_printf)
 * @param ap the argument-list
 */
void test_caseStartv(const char *fmt,va_list ap);

/**
 * Reports that a test-case was successfull.
 */
void test_caseSucceeded(void);

/**
 * Reports that a test-case has failed. Prints the given string with arguments (see vid_printf).
 *
 * @param fmt the format of the string
 */
void test_caseFailed(const char *fmt,...);

/**
 * Checks whether the given argument is true/false. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received your received value
 * @param func the function-name
 * @param line the line
 * @return true if received is true
 */
bool test_doAssertTrue(bool received,const char *func,int line);
bool test_doAssertFalse(bool received,const char *func,int line);

/**
 * Checks whether the given values are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @param func the function-name
 * @param line the line
 * @return true if the integers are equal
 */
bool test_doAssertInt(int received,int expected,const char *func,int line);
bool test_doAssertUInt(uint received,uint expected,const char *func,int line);
bool test_doAssertLInt(long received,long expected,const char *func,int line);
bool test_doAssertULInt(ulong received,ulong expected,const char *func,int line);
bool test_doAssertLLInt(llong received,llong expected,const char *func,int line);
bool test_doAssertULLInt(ullong received,ullong expected,const char *func,int line);
bool test_doAssertPtr(const void *received,const void *expected,const char *func,int line);
bool test_doAssertUIntPtr(uintptr_t received,uintptr_t expected,const char *func,int line);
bool test_doAssertSize(size_t received,size_t expected,const char *func,int line);
bool test_doAssertSSize(ssize_t received,ssize_t expected,const char *func,int line);
bool test_doAssertOff(off_t received,off_t expected,const char *func,int line);
bool test_doAssertStr(const char *received,const char *expected,const char *func,int line);
#ifndef IN_KERNEL
bool test_doAssertFloat(float received,float expected,const char *func,int line);
bool test_doAssertDouble(double received,double expected,const char *func,int line);
#endif

/**
 * Registers the given test-module to the test-framework
 *
 * @param mod the module
 */
void test_register(sTestModule *mod);

/**
 * Runs all registered tests
 */
void test_start(void);

#ifdef __cplusplus
}
#endif
