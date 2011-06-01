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
#include <stdlib.h>
#include <map>
#include <string>

using namespace std;

/* forward declarations */
static void test_map(void);
static void test_insert(void);
static void test_operators(void);
static void test_bounds(void);

/* our test-module */
sTestModule tModMap = {
	"Map",
	&test_map
};

static void test_map(void) {
	test_insert();
	test_operators();
	test_bounds();
}

static void test_insert(void) {
	test_caseStart("Testing insert");

	map<int,int> m;
	m[4] = 2;
	m[1] = -12;

	test_caseSucceeded();
}

static void test_operators(void) {
	test_caseStart("Testing operators");

	map<string,int> m1;
	m1["foo"] = 1;
	m1["bar"] = 4;
	m1["a"] = 12;
	m1["abcdef"] = 142;
	map<string,int> m2;
	m2["foo"] = 1;
	m2["bar"] = 4;
	m2["a"] = 12;
	m2["abcdef"] = 142;
	test_assertTrue(m1 == m2);

	map<string,int> m3;
	m3["a"] = 1;
	m3["b"] = 4;
	m3["c"] = 12;
	m3["d"] = 142;
	map<string,int> m4;
	m4["a"] = 1;
	m4["b"] = 4;
	m4["f"] = 12;
	m4["d"] = 142;
	test_assertTrue(m3 < m4);

	test_caseSucceeded();
}

static void test_bounds(void) {
	test_caseStart("Testing bounds");

	map<int,int> m;
	m[1] = 1;
	m[3] = 3;
	m[5] = 5;
	m[7] = 7;
	test_assertInt((*m.lower_bound(2)).first,3);
	test_assertInt((*m.lower_bound(3)).first,3);
	test_assertInt((*m.upper_bound(2)).first,3);
	test_assertInt((*m.upper_bound(3)).first,5);
	test_assertTrue(m.lower_bound(8) == m.end());
	test_assertTrue(m.upper_bound(8) == m.end());

	test_caseSucceeded();
}
