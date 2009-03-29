/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <env.h>
#include <test.h>
#include <stdarg.h>
#include "tenv.h"

/* forward declarations */
static void test_env(void);
static void test_setget(void);
static void test_geti(void);

/* our test-module */
sTestModule tModEnv = {
	"Env",
	&test_env
};

static void test_env(void) {
	test_setget();
	test_geti();
}

static void test_setget(void) {
	test_caseStart("Testing setEnv() and getEnv()");
	test_assertInt(setEnv("TEST","my value"),0);
	test_assertStr(getEnv("TEST"),"my value");
	test_assertInt(setEnv("A","123"),0);
	test_assertStr(getEnv("A"),"123");
	test_caseSucceded();
}

static void test_geti(void) {
	u32 i;
	char *name;
	test_caseStart("Testing getEnvByIndex()");

	for(i = 0; ; i++) {
		name = getEnvByIndex(i);
		if(name == NULL)
			break;
		test_assertTrue(getEnv(name) != NULL);
	}

	test_caseSucceded();
}
