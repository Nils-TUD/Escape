/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef TEST_H_
#define TEST_H_

#include <types.h>
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
#define test_assertInt(recv,exp) test_doAssertInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertUInt(recv,exp) test_doAssertUInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertLInt(recv,exp) test_doAssertLInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertULInt(recv,exp) test_doAssertULInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertStr(recv,exp) test_doAssertStr((recv),(exp),__FUNCTION__,__LINE__)

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
 * Starts a test-case with given name. This ends with a call of test_caseSucceded() or
 * test_caseFailed().
 *
 * @param fmt the format of the string to print (see vid_printf)
 */
void test_caseStart(const char *fmt,...);

/**
 * Starts a test-case with given name. This ends with a call of test_caseSucceded() or
 * test_caseFailed().
 *
 * @param fmt the format of the string to print (see vid_printf)
 * @param ap the argument-list
 */
void test_caseStartv(const char *fmt,va_list ap);

/**
 * Reports that a test-case was successfull.
 */
void test_caseSucceded(void);

/**
 * Reports that a test-case has failed. Prints the given string with arguments (see vid_printf).
 *
 * @param fmt the format of the string
 */
void test_caseFailed(const char *fmt,...);

/**
 * Checks wether the given argument is true. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received your received value
 * @param func the function-name
 * @param line the line
 * @return true if received is true
 */
bool test_doAssertTrue(bool received,const char *func,u32 line);

/**
 * Checks wether the given argument is false. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received your received value
 * @param func the function-name
 * @param line the line
 * @return true if received is false
 */
bool test_doAssertFalse(bool received,const char *func,u32 line);

/**
 * Checks wether the given pointers are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @param func the function-name
 * @param line the line
 * @return true if the pointers are equal
 */
bool test_doAssertPtr(const void *received,const void *expected,const char *func,u32 line);

/**
 * Checks wether the given integers are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @param func the function-name
 * @param line the line
 * @return true if the integers are equal
 */
bool test_doAssertInt(s32 received,s32 expected,const char *func,u32 line);

/**
 * Checks wether the given integers are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @param func the function-name
 * @param line the line
 * @return true if the integers are equal
 */
bool test_doAssertUInt(u32 received,u32 expected,const char *func,u32 line);

/**
 * Checks wether the given long integers are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @param func the function-name
 * @param line the line
 * @return true if the integers are equal
 */
bool test_doAssertLInt(s64 received,s64 expected,const char *func,u32 line);

/**
 * Checks wether the given unsigned long integers are equal. If not it calls test_caseFailed()
 * and returns false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @param func the function-name
 * @param line the line
 * @return true if the integers are equal
 */
bool test_doAssertULInt(u64 received,u64 expected,const char *func,u32 line);

/**
 * Checks wether the given strings are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @param func the function-name
 * @param line the line
 * @return true if the strings are equal
 */
bool test_doAssertStr(const char *received,const char *expected,const char *func,u32 line);

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

#endif /* TEST_H_ */
