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

#include <esc/common.h>
#include <esc/env.h>
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
