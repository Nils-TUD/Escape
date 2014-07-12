/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <sys/test.h>
#include <stdlib.h>
#include <stdarg.h>

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
	char *value;
	test_caseStart("Testing setenv() and getenv()");
	test_assertInt(setenv("TEST","my value"),0);
	test_assertTrue((value = getenv("TEST")) != NULL);
	test_assertStr(value,"my value");
	test_assertInt(setenv("A","123"),0);
	test_assertTrue((value = getenv("A")) != NULL);
	test_assertStr(value,"123");
	test_caseSucceeded();
}

static void test_geti(void) {
	size_t i;
	test_caseStart("Testing getenvi()");

	for(i = 0; ; i++) {
		char *name = getenvi(i);
		if(!name)
			break;
		test_assertTrue(getenv(name) != NULL);
	}

	test_caseSucceeded();
}
