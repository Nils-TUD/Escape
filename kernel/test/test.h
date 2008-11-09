/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef TEST_H_
#define TEST_H_

#include "../h/common.h"
#include <stdarg.h>

#define MAX_TESTS 100
#define MAX_TEST_CASES 20

typedef void (*startFunc)(void);

/* one test-module */
typedef struct {
	cstring name;
	startFunc start;
} tTestModule;

#ifdef TESTSQUIET
#define tprintf test_noPrint
#define tvprintf test_vnoPrint
#else
#define tprintf vid_printf
#define tvprintf vid_vprintf
#endif

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
 * Registers the given test-module to the test-framework
 *
 * @param mod the module
 */
void test_register(tTestModule *mod);

/**
 * Runs all registered tests
 */
void test_start(void);

#endif /* TEST_H_ */
