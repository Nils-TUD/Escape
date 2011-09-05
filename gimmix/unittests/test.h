/**
 * $Id: test.h 146 2011-04-04 17:09:09Z nasmussen $
 */

#ifndef TEST_H_
#define TEST_H_

#include <stdarg.h>
#include <stdbool.h>
#include "common.h"

typedef void (*fTestFunc)(void);

typedef struct {
	const char *name;
	fTestFunc func;
} sTestCase;

// some macros for convenience and better error-reporting
#define test_assertTrue(val) test_doAssertTrue((val),__FUNCTION__,__LINE__)
#define test_assertFalse(val) test_doAssertFalse((val),__FUNCTION__,__LINE__)
#define test_assertPtr(recv,exp) test_doAssertPtr((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertInt(recv,exp) test_doAssertInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertUInt(recv,exp) test_doAssertUInt((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertByte(recv,exp) test_doAssertByte((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertWyde(recv,exp) test_doAssertWyde((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertTetra(recv,exp) test_doAssertTetra((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertOcta(recv,exp) test_doAssertOcta((recv),(exp),__FUNCTION__,__LINE__)
#define test_assertStr(recv,exp) test_doAssertStr((recv),(exp),__FUNCTION__,__LINE__)

/**
 * Checks whether <received> is true/false. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param func the function-name
 * @param line the line
 * @return true if its true/false
 */
bool test_doAssertTrue(bool received,const char *func,int line);
bool test_doAssertFalse(bool received,const char *func,int line);

/**
 * Checks whether <received> is equal to <expected>. If not it calls test_caseFailed() and returns
 * false.
 *
 * @param received the result you received
 * @param expected your expected result
 * @param func the function-name
 * @param line the line
 * @return true if they are equal
 */
bool test_doAssertPtr(const void *received,const void *expected,const char *func,int line);
bool test_doAssertInt(int received,int expected,const char *func,int line);
bool test_doAssertUInt(unsigned int received,unsigned int expected,const char *func,int line);
bool test_doAssertByte(byte received,byte expected,const char *func,int line);
bool test_doAssertWyde(wyde received,wyde expected,const char *func,int line);
bool test_doAssertTetra(tetra received,tetra expected,const char *func,int line);
bool test_doAssertOcta(octa received,octa expected,const char *func,int line);
bool test_doAssertStr(const char *received,const char *expected,const char *func,int line);

/**
 * Registers the given test-suite to the test-framework
 *
 * @param name the name (will NOT be copied)
 * @param cases a NULL-terminated array of test-cases (will NOT be copied)
 */
void test_register(const char *name,const sTestCase *cases);

/**
 * Runs all registered tests
 */
void test_start(void);

/**
 * Free's the allocated memory
 */
void test_cleanup(void);

#endif /* TEST_H_ */
