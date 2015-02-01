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
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void test_string(void);
static void test_strtold(void);
static void test_ecvt(void);

/* our test-module */
sTestModule tModString = {
	"String",
	&test_string
};

static void test_string(void) {
	test_strtold();
	test_ecvt();
}

static void test_strtold(void) {
	struct StrtoldTest {
		const char *str;
		long double res;
	};
	size_t i;
	long double res;
	char *end;
	test_caseStart("Testing strtold()");

	{
		struct StrtoldTest tests[] = {
			{"1234",		1234L},
			{" 12.34",		12.34L},
			{".5",			.5L},
			{" INF",		INFINITY},
			{"-infINITY",	-INFINITY},
			{"+6.0e2",		6.0e2L},
			{"-12.34E10",	-12.34E10L},
			{"0xABC.DEp2",	0xABC.DEp2L},
			{"0xA.",		0xAL},
		};

		for(i = 0; i < ARRAY_SIZE(tests); i++) {
			res = strtold(tests[i].str,NULL);
			test_assertTrue(res == tests[i].res);
			res = strtold(tests[i].str,&end);
			test_assertTrue(res == tests[i].res);
			test_assertTrue(*end == '\0');
		}
	}

	// don't compare nans with directly with ==
	{
		struct StrtoldTest tests[] = {
			{"NAN",			NAN},
			{"  \tnan",		NAN},
		};

		for(i = 0; i < ARRAY_SIZE(tests); i++) {
			res = strtold(tests[i].str,NULL);
			test_assertTrue(isnan(res) == isnan(tests[i].res));
			res = strtold(tests[i].str,&end);
			test_assertTrue(isnan(res) == isnan(tests[i].res));
			test_assertTrue(*end == '\0');
		}
	}

	test_caseSucceeded();
}

static void test_ecvt(void) {
	int decpt,sign;
	char *s;
	test_caseStart("Testing ecvt()");

	s = ecvt(1234.5678,3,&decpt,&sign);
	test_assertStr(s,"123");
	test_assertInt(decpt,4);
	test_assertInt(sign,0);

	s = ecvt(0,1,&decpt,&sign);
	test_assertStr(s,"");
	test_assertInt(decpt,0);
	test_assertInt(sign,0);

	s = ecvt(0.23,4,&decpt,&sign);
	test_assertStr(s,"2300");
	test_assertInt(decpt,0);
	test_assertInt(sign,0);

	s = ecvt(-10.33,12,&decpt,&sign);
	test_assertStr(s,"103300000000");
	test_assertInt(decpt,2);
	test_assertInt(sign,1);

	test_caseSucceeded();
}
