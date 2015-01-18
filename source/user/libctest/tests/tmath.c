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
#include <assert.h>
#include <math.h>

static void test_math(void);
static void test_round(void);
static void test_roundf(void);

/* our test-module */
sTestModule tModMath = {
	"Math",
	&test_math
};

static void test_math(void) {
	test_round();
	test_roundf();
}

static void test_round(void) {
	test_caseStart("Testing round()");

	test_assertDouble(round(0.1),0.0);
	test_assertDouble(round(0.49),0.0);
	test_assertDouble(round(0.8),1.0);
	test_assertDouble(round(1.2),1.0);
	test_assertDouble(round(101.9),102.0);
	test_assertDouble(round(-3.1),-3.0);
	test_assertDouble(round(-10.6),-11.0);

	test_caseSucceeded();
}

static void test_roundf(void) {
	test_caseStart("Testing roundf()");

	test_assertFloat(roundf(0.1f),0.0f);
	test_assertFloat(roundf(0.49f),0.0f);
	test_assertFloat(roundf(0.8f),1.0f);
	test_assertFloat(roundf(1.2f),1.0f);
	test_assertFloat(roundf(101.9f),102.0f);
	test_assertFloat(roundf(-3.1f),-3.0f);
	test_assertFloat(roundf(-10.6f),-11.0f);

	test_caseSucceeded();
}
