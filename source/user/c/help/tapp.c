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
#include <esc/test.h>
#include "app.h"
#include "tapp.h"

static void test_app(void);
static void test_app_1(void);
static void test_app_2(void);

static const char *app1 =
	"name:					\"muh\""
	"desc:					\"mydesc\""
	"start:					\"mystart\""
	"type:					\"user\"";

static const char *app2 =
	"name:					\"myapppp\""
	"desc:					\"a b c\""
	"start:					\"\""
	"type:					\"driver\"";

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
	char *res;
	sApp a;
	test_caseStart("Parsing, toString(), parsing again");

	res = app_fromString(app1,&a);
	test_assertTrue(res != NULL);
	test_assertStr(a.name,"muh");
	test_assertStr(a.desc,"mydesc");
	test_assertStr(a.start,"mystart");
	test_assertUInt(a.type,APP_TYPE_USER);

	test_caseSucceeded();
}

static void test_app_2(void) {
	char *res;
	sApp a;
	test_caseStart("Parsing, toString(), parsing again");

	res = app_fromString(app2,&a);
	test_assertTrue(res != NULL);
	test_assertStr(a.name,"myapppp");
	test_assertStr(a.desc,"a b c");
	test_assertStr(a.start,"");
	test_assertUInt(a.type,APP_TYPE_DRIVER);
	test_caseSucceeded();
}
