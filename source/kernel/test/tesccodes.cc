/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include "tesccodes.h"
#include <esc/esccodes.h>
#include <esc/test.h>

/* forward declarations */
static void test_esccodes();
static void test_check(const char *str,int cmd,int n1,int n2,int n3);
static void test_1();
static void test_2();
static void test_3();

/* our test-module */
sTestModule tModEscCodes = {
	"Escape-Codes",
	&test_esccodes
};

static void test_esccodes() {
	test_1();
	test_2();
	test_3();
}

static void test_check(const char *str,int cmd,int n1,int n2,int n3) {
	int rn1,rn2,rn3;
	const char *s = str + 1;
	int rcmd = escc_get(&s,&rn1,&rn2,&rn3);
	test_assertInt(rcmd,cmd);
	if(cmd == ESCC_INCOMPLETE)
		test_assertInt(*s,str[1]);
	else if(cmd != ESCC_INVALID)
		test_assertInt(*s,'\0');
	/* just to make it easy to figure out where an error occurred ;) */
	else
		test_assertTrue(true);
	if(cmd != ESCC_INCOMPLETE) {
		test_assertInt(rn1,n1);
		test_assertInt(rn2,n2);
		test_assertInt(rn3,n3);
	}
	else {
		test_assertTrue(true);
		test_assertTrue(true);
		test_assertTrue(true);
	}
}

static void test_1() {
	const char *str[] = {
		"\033[ml]","\033[ml;12]",
		"\033[mr]","\033[mr;0]",
		"\033[mu]","\033[mu;1]",
		"\033[md]","\033[md;6]",
		"\033[mh]",
		"\033[ms]",
		"\033[me]",
		"\033[kc]","\033[kc;0;0]","\033[kc;12345;12345]",
		"\033[kc;1;2;3]","\033[kc;4;5;6]","\033[kc;123;123;123]",
		"\033[co]","\033[co;12;441]","\033[co;123;123]",
	};
	test_caseStart("Valid codes");

	test_check(str[0],ESCC_MOVE_LEFT,1,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[1],ESCC_MOVE_LEFT,12,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[2],ESCC_MOVE_RIGHT,1,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[3],ESCC_MOVE_RIGHT,0,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[4],ESCC_MOVE_UP,1,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[5],ESCC_MOVE_UP,1,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[6],ESCC_MOVE_DOWN,1,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[7],ESCC_MOVE_DOWN,6,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[8],ESCC_MOVE_HOME,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[9],ESCC_MOVE_LINESTART,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[10],ESCC_MOVE_LINEEND,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[11],ESCC_KEYCODE,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[12],ESCC_KEYCODE,0,0,ESCC_ARG_UNUSED);
	test_check(str[13],ESCC_KEYCODE,12345,12345,ESCC_ARG_UNUSED);
	test_check(str[14],ESCC_KEYCODE,1,2,3);
	test_check(str[15],ESCC_KEYCODE,4,5,6);
	test_check(str[16],ESCC_KEYCODE,123,123,123);
	test_check(str[17],ESCC_COLOR,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[18],ESCC_COLOR,12,441,ESCC_ARG_UNUSED);
	test_check(str[19],ESCC_COLOR,123,123,ESCC_ARG_UNUSED);

	test_caseSucceeded();
}

static void test_2() {
	size_t i;
	const char *str[] = {
		"\033[]","\033[ab]","\033[ab;]","\033[;]","\033[;;]","\033[;;;]","\033[;bb;a]",
		"\033]]","\033[colorcodeandmore]","\033[co;123123123123123123;12312313]","\033[co;+23,-12]",
	};
	test_caseStart("Invalid codes");

	for(i = 0; i < ARRAY_SIZE(str); i++)
		test_check(str[i],ESCC_INVALID,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);

	test_caseSucceeded();
}

static void test_3() {
	size_t i;
	const char *str[] = {
		"\033[","\033[c","\033[co","\033[co;","\033[co;1","\033[co;12","\033[co;12;","\033[co;12;3"
	};
	test_caseStart("Incomplete codes");

	for(i = 0; i < ARRAY_SIZE(str); i++)
		test_check(str[i],ESCC_INCOMPLETE,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);

	test_caseSucceeded();
}
