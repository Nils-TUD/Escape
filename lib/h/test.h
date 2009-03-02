/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef TEST_H_
#define TEST_H_

#ifdef IN_KERNEL
#	include "../../kernel/h/common.h"
#else
#	include "../../libc/h/common.h"
#endif

#include <stdarg.h>

#define MAX_TESTS 100
#define MAX_TEST_CASES 20

typedef void (*fStart)(void);

/* one test-module */
typedef struct {
	cstring name;
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
#		define tprintf debugf
#		define tvprintf vdebugf
#	endif
#endif

/**
 * The "do-nothing" printf-function
 */
void test_noPrint(cstring fmt,...);
/**
 * The "do-nothing" vprintf-function
 */
void test_vnoPrint(cstring fmt,va_list ap);

/**
 * Starts a test-case with given name. This ends with a call of test_caseSucceded() or
 * test_caseFailed().
 *
 * @param fmt the format of the string to print (see vid_printf)
 */
void test_caseStart(cstring fmt,...);

/**
 * Starts a test-case with given name. This ends with a call of test_caseSucceded() or
 * test_caseFailed().
 *
 * @param fmt the format of the string to print (see vid_printf)
 * @param ap the argument-list
 */
void test_caseStartv(cstring fmt,va_list ap);

/**
 * Reports that a test-case was successfull.
 */
void test_caseSucceded(void);

/**
 * Reports that a test-case has failed. Prints the given string with arguments (see vid_printf).
 *
 * @param fmt the format of the string
 */
void test_caseFailed(cstring fmt,...);

/**
 * Checks wether the given argument is true. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received your received value
 * @return true if received is true
 */
bool test_assertTrue(bool received);

/**
 * Checks wether the given argument is false. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received your received value
 * @return true if received is false
 */
bool test_assertFalse(bool received);

/**
 * Checks wether the given pointers are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @return true if the pointers are equal
 */
bool test_assertPtr(void *received,void *expected);

/**
 * Checks wether the given integers are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @return true if the integers are equal
 */
bool test_assertInt(s32 received,s32 expected);

/**
 * Checks wether the given integers are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @return true if the integers are equal
 */
bool test_assertUInt(u32 received,u32 expected);

/**
 * Checks wether the given strings are equal. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @return true if the strings are equal
 */
bool test_assertStr(string received,string expected);

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

#endif /* TEST_H_ */
