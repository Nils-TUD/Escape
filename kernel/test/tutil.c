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
#include <util.h>

#include "tutil.h"
#include <test.h>

/* forward declarations */
static void test_util(void);
static void test_util_sprintfLen(void);

/* our test-module */
sTestModule tModUtil = {
	"Util",
	&test_util
};

static void test_util(void) {
	test_util_sprintfLen();
}

static void test_util_sprintfLen(void) {
	test_caseStart("Testing util_sprintfLen()");

	test_assertUInt(util_sprintfLen(""),0);
	test_assertUInt(util_sprintfLen("a"),1);
	test_assertUInt(util_sprintfLen("abc"),3);
	test_assertUInt(util_sprintfLen("%d",4),1);
	test_assertUInt(util_sprintfLen("%d",-123),4);
	test_assertUInt(util_sprintfLen("%5d",-123),5);
	test_assertUInt(util_sprintfLen("%x",0x123),3);
	test_assertUInt(util_sprintfLen("%8x",0x123),8);
	test_assertUInt(util_sprintfLen("%u",78189234),8);
	test_assertUInt(util_sprintfLen("%9u",78189234),9);
	test_assertUInt(util_sprintfLen("%o",01),1);
	test_assertUInt(util_sprintfLen("%4o",01),4);
	test_assertUInt(util_sprintfLen("%b",0x123),9);
	test_assertUInt(util_sprintfLen("%12b",0x123),12);
	test_assertUInt(util_sprintfLen("%s",""),0);
	test_assertUInt(util_sprintfLen("%s","test"),4);
	test_assertUInt(util_sprintfLen("%10s","test"),10);
	test_assertUInt(util_sprintfLen("%c",'a'),1);
	test_assertUInt(util_sprintfLen("%%"),1);
	test_assertUInt(util_sprintfLen("1:%d,2:%-5d,3:hier,4:%-16x",3,-123,0xDEADBEEF),37);
	test_assertUInt(util_sprintfLen("1:%d,2:%-2d,3:hier,4:%-4x",3,-123,0xDEADBEEF),28);

	test_caseSucceded();
}
