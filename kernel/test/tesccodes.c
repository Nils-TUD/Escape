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
#include "tesccodes.h"
#include <esccodes.h>
#include <test.h>

/* forward declarations */
static void test_esccodes(void);
static void test_check(const char *str,s32 cmd,s32 n1,s32 n2);
static void test_1(void);
static void test_2(void);
static void test_3(void);

/* our test-module */
sTestModule tModEscCodes = {
	"Escape-Codes",
	&test_esccodes
};

static void test_esccodes(void) {
	test_1();
	test_2();
	test_3();
}

static void test_check(const char *str,s32 cmd,s32 n1,s32 n2) {
	s32 rn1,rn2;
	const char *s = str + 1;
	u32 rcmd = escc_get(&s,&rn1,&rn2);
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
	}
	else {
		test_assertTrue(true);
		test_assertTrue(true);
	}
}

static void test_1(void) {
	const char *str[] = {
		"\033[ml]","\033[ml;12]","\033[ml;12;441]",
		"\033[mr]","\033[mr;0]","\033[mr;0;0]",
		"\033[mu]","\033[mu;1]","\033[mu;123;123]",
		"\033[md]","\033[md;6]","\033[md;123456789;123456789]",
		"\033[mh]",
		"\033[ms]",
		"\033[me]",
		"\033[kc]",
		"\033[co]",
	};
	test_caseStart("Valid codes");

	test_check(str[0],ESCC_MOVE_LEFT,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[1],ESCC_MOVE_LEFT,12,ESCC_ARG_UNUSED);
	test_check(str[2],ESCC_MOVE_LEFT,12,441);
	test_check(str[3],ESCC_MOVE_RIGHT,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[4],ESCC_MOVE_RIGHT,0,ESCC_ARG_UNUSED);
	test_check(str[5],ESCC_MOVE_RIGHT,0,0);
	test_check(str[6],ESCC_MOVE_UP,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[7],ESCC_MOVE_UP,1,ESCC_ARG_UNUSED);
	test_check(str[8],ESCC_MOVE_UP,123,123);
	test_check(str[9],ESCC_MOVE_DOWN,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[10],ESCC_MOVE_DOWN,6,ESCC_ARG_UNUSED);
	test_check(str[11],ESCC_MOVE_DOWN,123456789,123456789);
	test_check(str[12],ESCC_MOVE_HOME,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[13],ESCC_MOVE_LINESTART,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[14],ESCC_MOVE_LINEEND,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[15],ESCC_KEYCODE,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);
	test_check(str[16],ESCC_COLOR,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);

	test_caseSucceded();
}

static void test_2(void) {
	u32 i;
	const char *str[] = {
		"\033[]","\033[ab]","\033[ab;]","\033[;]","\033[;;]","\033[;;;]","\033[;bb;a]",
		"\033]]","\033[colorcodeandmore]","\033[co;+23,-12]",
	};
	test_caseStart("Invalid codes");

	for(i = 0; i < ARRAY_SIZE(str); i++)
		test_check(str[i],ESCC_INVALID,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);

	test_caseSucceded();
}

static void test_3(void) {
	u32 i;
	const char *str[] = {
		"\033[","\033[c","\033[co","\033[co;","\033[co;1","\033[co;12","\033[co;12;","\033[co;12;3"
	};
	test_caseStart("Incomplete codes");

	for(i = 0; i < ARRAY_SIZE(str); i++)
		test_check(str[i],ESCC_INCOMPLETE,ESCC_ARG_UNUSED,ESCC_ARG_UNUSED);

	test_caseSucceded();
}
