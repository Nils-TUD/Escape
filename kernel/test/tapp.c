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

#include <common.h>
#include <app.h>
#include "tapp.h"
#include <test.h>

static void test_app(void);
static void test_app_1(void);
static void test_app_2(void);

static const char *app1 =
	"name:					\"muh\";"
	"desc:					\"mydesc\";";

static const char *app2 =
	"name:					\"myapppp\";"
	"source:				\"a b c\";";

/* our test-module */
sTestModule tModApp = {
	"App",
	&test_app
};

static void test_app(void) {
	test_app_1();
	test_app_2();
}

static void test_app_1(void) {
	u32 i;
	char *res;
	sApp a;
	sStringBuffer buf;
	buf.dynamic = true;
	buf.len = 0;
	buf.size = 0;
	buf.str = NULL;
	test_caseStart("Parsing, toString(), parsing again");

	res = app_fromString(app1,&a);
	test_assertTrue(res != NULL);
	for(i = 0; i < 2; i++) {
		test_assertStr(a.name,"muh");
		test_assertStr(a.desc,"mydesc");

		/* create str from it and parse again */
		if(i < 1) {
			test_assertTrue(app_toString(&buf,&a));
			test_assertTrue(app_fromString(buf.str,&a) != NULL);
		}
	}

	test_caseSucceded();
}

static void test_app_2(void) {
	u32 i;
	char *res;
	sApp a;
	sStringBuffer buf;
	buf.dynamic = true;
	buf.len = 0;
	buf.size = 0;
	buf.str = NULL;
	test_caseStart("Parsing, toString(), parsing again");

	res = app_fromString(app2,&a);
	test_assertTrue(res != NULL);
	for(i = 0; i < 2; i++) {
		test_assertStr(a.name,"myapppp");
		test_assertStr(a.desc,"a b c");

		/* create str from it and parse again */
		if(i == 0) {
			test_assertTrue(app_toString(&buf,&a));
			test_assertTrue(app_fromString(buf.str,&a) != NULL);
		}
	}
	test_caseSucceded();
}
