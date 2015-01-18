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

#include <gui/event/subscriber.h>
#include <sys/common.h>
#include <sys/test.h>
#include <tuple>

using namespace std;
using namespace gui;

static size_t calls[15];

static void test_func0(int a) {
	test_assertInt(a,1);
	calls[0]++;
}
static void test_func1(int a,int b) {
	test_assertInt(a,2);
	test_assertInt(b,1);
	calls[1]++;
}
static void test_func2(tuple<int> &t,int b) {
	test_assertInt(get<0>(t),2);
	test_assertInt(b,1);
	calls[2]++;
}
static void test_func3(const tuple<int> &t,int b) {
	test_assertInt(get<0>(t),2);
	test_assertInt(b,1);
	calls[3]++;
}

static void test_func4(int& a) {
	test_assertInt(a,1);
	calls[4]++;
}
static void test_func5(tuple<int> &t,int& b) {
	test_assertInt(get<0>(t),2);
	test_assertInt(b,1);
	calls[5]++;
}

class Test {
public:
	void test_func6(int a) {
		test_assertInt(a,1);
		calls[6]++;
	}
	void test_func7(int a,int b) {
		test_assertInt(a,2);
		test_assertInt(b,1);
		calls[7]++;
	}
	void test_func8(tuple<int> &t,int b) {
		test_assertInt(get<0>(t),2);
		test_assertInt(b,1);
		calls[8]++;
	}
	void test_func9(const tuple<int> &t,int b) {
		test_assertInt(get<0>(t),2);
		test_assertInt(b,1);
		calls[9]++;
	}
	void test_func10(tuple<int> &t,int b) const {
		test_assertInt(get<0>(t),2);
		test_assertInt(b,1);
		calls[10]++;
	}
	void test_func11(const tuple<int> &t,int b) const {
		test_assertInt(get<0>(t),2);
		test_assertInt(b,1);
		calls[11]++;
	}

	void test_func12(int& a) {
		test_assertInt(a,1);
		calls[12]++;
	}
	void test_func13(tuple<int> &t,int& b) {
		test_assertInt(get<0>(t),2);
		test_assertInt(b,1);
		calls[13]++;
	}

	void test_func14(tuple<int> &t,int& b) const {
		test_assertInt(get<0>(t),2);
		test_assertInt(b,1);
		calls[14]++;
	}
};

/* forward declarations */
static void test_subscriber(void);
static void test_func(void);
static void test_memfunc(void);

/* our test-module */
sTestModule tModSubscriber = {
	"Subscriber",
	&test_subscriber
};

static void test_subscriber(void) {
	test_func();
	test_memfunc();
}

static void test_func(void) {
	test_caseStart("Testing functions");

	{
		Sender<int> s;
		tuple<int> x(2);
		s.subscribe(func_recv(test_func0));
		s.subscribe(bind1_func_recv(2,test_func1));
		s.subscribe(bind1_func_recv(x,test_func2));
		s.subscribe(bind1_func_recv(const_cast<const tuple<int>&>(x),test_func3));
		s.send(1);
		for(size_t i = 0; i <= 3; ++i)
			test_assertSize(calls[i],1);
	}

	{
		Sender<int&> s;
		tuple<int> x(2);
		s.subscribe(func_recv(test_func4));
		s.subscribe(bind1_func_recv(x,test_func5));
		int y = 1;
		s.send(y);
		for(size_t i = 4; i <= 5; ++i)
			test_assertSize(calls[i],1);
	}

	test_caseSucceeded();
}

static void test_memfunc(void) {
	test_caseStart("Testing member functions");

	{
		Test test;
		const Test ctest;
		Sender<int> s;
		tuple<int> x(2);
		s.subscribe(mem_recv(&test,&Test::test_func6));
		s.subscribe(bind1_mem_recv(2,&test,&Test::test_func7));
		s.subscribe(bind1_mem_recv(x,&test,&Test::test_func8));
		s.subscribe(bind1_mem_recv(const_cast<const tuple<int>&>(x),&test,&Test::test_func9));
		s.subscribe(bind1_mem_recv(x,&ctest,&Test::test_func10));
		s.subscribe(bind1_mem_recv(const_cast<const tuple<int>&>(x),&ctest,&Test::test_func11));
		s.send(1);
		for(size_t i = 6; i <= 11; ++i)
			test_assertSize(calls[i],1);
	}

	{
		Test test;
		const Test ctest;
		Sender<int&> s;
		tuple<int> x(2);
		s.subscribe(mem_recv(&test,&Test::test_func12));
		s.subscribe(bind1_mem_recv(x,&test,&Test::test_func13));
		s.subscribe(bind1_mem_recv(x,&ctest,&Test::test_func14));
		int y = 1;
		s.send(y);
		for(size_t i = 12; i <= 14; ++i)
			test_assertSize(calls[i],1);
	}

	test_caseSucceeded();
}
