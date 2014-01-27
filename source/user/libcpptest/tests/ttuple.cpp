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

#include <esc/common.h>
#include <esc/test.h>
#include <stdlib.h>
#include <tuple>
#include <vector>

using namespace std;

/* forward declarations */
static void test_tuple(void);
static void test_basic(void);

class OnlyMoveable {
public:
	OnlyMoveable() {
	}
	OnlyMoveable(const OnlyMoveable&) = delete;
	OnlyMoveable(OnlyMoveable&&) {
	}
	OnlyMoveable &operator=(const OnlyMoveable&) = delete;
	OnlyMoveable &operator=(OnlyMoveable&&) {
		return *this;
	}
};

/* our test-module */
sTestModule tModTuple = {
	"Tuple",
	&test_tuple
};

static void test_tuple(void) {
	test_basic();
}

static void test_basic(void) {
	test_caseStart("Testing basic operations");

	{
		tuple<int> t1;
		tuple<int,int> t2;
		tuple<int,int,int> t3;
		tuple<int,float,double> t4;

		static_assert(tuple_size<decltype(t1)>::value == 1,"Failed");
		static_assert(tuple_size<decltype(t2)>::value == 2,"Failed");
		static_assert(tuple_size<decltype(t3)>::value == 3,"Failed");
		static_assert(tuple_size<decltype(t4)>::value == 3,"Failed");
	}

	{
		tuple<int> t1(4);
		tuple<int,int> t2(4,6);
		tuple<int,int,int> t3(1,2,3);
		tuple<int,float,double> t4(1,4.3f,0.1);
		tuple<int,int> t5;
		t5 = t2;

		test_assertInt(get<0>(t1),4);
		test_assertInt(get<0>(t2),4);
		test_assertInt(get<1>(t2),6);
		test_assertInt(get<0>(t3),1);
		test_assertInt(get<1>(t3),2);
		test_assertInt(get<2>(t3),3);
		test_assertInt(get<0>(t4),1);
		test_assertFloat(get<1>(t4),4.3f);
		test_assertDouble(get<2>(t4),0.1);
		test_assertInt(get<0>(t5),4);
		test_assertInt(get<0>(t5),4);
	}

	{
		tuple<OnlyMoveable,int> t1(OnlyMoveable(),124);
		tuple<OnlyMoveable,int> t2;
		t2 = tuple<OnlyMoveable,int>(OnlyMoveable(),10);

		test_assertInt(get<1>(t1),124);
		test_assertInt(get<1>(t2),10);
	}

	{
		pair<int,float> p1(10,20);
		tuple<int,float> t1(p1);
		tuple<int,OnlyMoveable> t2(pair<int,OnlyMoveable>(4,OnlyMoveable()));
		tuple<int,float> t3(1,4.0),t5,t6;
		tuple<int,int> t4(1,2);
		t3 = t4;
		t4 = pair<int,int>(1,2);
		t5 = tuple<int,int>(1,3);
		t6 = p1;

		test_assertInt(get<0>(t1),10);
		test_assertFloat(get<1>(t1),20);
		test_assertInt(get<0>(t2),4);
		test_assertInt(get<0>(t3),1);
		test_assertFloat(get<1>(t3),2.0f);
		test_assertInt(get<0>(t4),1);
		test_assertInt(get<1>(t4),2);
		test_assertInt(get<0>(t5),1);
		test_assertFloat(get<1>(t5),3.0f);
		test_assertInt(get<0>(t6),10);
		test_assertFloat(get<1>(t6),20.0f);
	}

	{
		tuple<int,float,int> t1 = make_tuple(1,2.0f,3);
		tuple<int,float,int> t2 = make_tuple(45,46.0f,47);
		t1.swap(t2);

		test_assertInt(get<0>(t1),45);
		test_assertFloat(get<1>(t1),46.0f);
		test_assertInt(get<2>(t1),47);
		test_assertInt(get<0>(t2),1);
		test_assertFloat(get<1>(t2),2.0f);
		test_assertInt(get<2>(t2),3);
	}

	test_caseSucceeded();
}
