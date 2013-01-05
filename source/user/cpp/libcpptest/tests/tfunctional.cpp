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

#include <esc/common.h>
#include <esc/test.h>
#include <stdlib.h>
#include <functional>
#include <algorithm>
#include <vector>
#include <utility>

using namespace std;

/* forward declarations */
static void test_functional(void);
static void test_arithmetic(void);
static void test_comparison(void);
static void test_logical(void);
static void test_adaptors(void);

// NOTE: be carefull that every argument is extacted as 'int'!
template<class T>
static void check_content(const vector<T> &l,size_t count,...) {
	va_list ap;
	va_start(ap,count);
	test_assertUInt(l.size(),count);
	for(auto it = l.begin(); it != l.end(); it++)
		test_assertInt(*it,va_arg(ap,int));
	va_end(ap);
}

/* our test-module */
sTestModule tModFunctional = {
	"Functional",
	&test_functional
};

static void test_functional(void) {
	test_arithmetic();
	test_comparison();
	test_logical();
	test_adaptors();
}

static void test_arithmetic(void) {
	test_caseStart("Testing arithmetical operators");

	{
		int first[] = {1,2,3,4,5};
		int second[] = {10,20,30,40,50};
		int results[5];
		transform(first,first + 5,second,results,plus<int>());
		vector<int> v(results,results + 5);
		check_content(v,5,11,22,33,44,55);
	}

	{
		int first[] = {1,2,3,4,5};
		int second[] = {10,20,30,40,50};
		int results[5];
		transform(first,first + 5,second,results,minus<int>());
		vector<int> v(results,results + 5);
		check_content(v,5,-9,-18,-27,-36,-45);
	}

	{
		int first[] = {1,2,3,4,5};
		int second[] = {10,20,30,40,50};
		int results[5];
		transform(first,first + 5,second,results,multiplies<int>());
		vector<int> v(results,results + 5);
		check_content(v,5,10,40,90,160,250);
	}

	{
		int first[] = {10,20,30,40,50};
		int second[] = {1,2,3,4,5};
		int results[5];
		transform(first,first + 5,second,results,divides<int>());
		vector<int> v(results,results + 5);
		check_content(v,5,10,10,10,10,10);
	}

	{
		int first[] = {10,20,30,40,50};
		int second[] = {1,3,7,9,11};
		int results[5];
		transform(first,first + 5,second,results,modulus<int>());
		vector<int> v(results,results + 5);
		check_content(v,5,0,2,2,4,6);
	}

	test_caseSucceeded();
}

static void test_comparison(void) {
	test_caseStart("Testing comparison operators");

	pair<int*,int*> ptiter;
	int foo[] = {10,20,30,40,50};
	int bar[] = {10,20,40,80,160};
	ptiter = mismatch(foo,foo + 5,bar,equal_to<int>());
	test_assertInt(*ptiter.first,30);
	test_assertInt(*ptiter.second,40);

	test_caseSucceeded();
}

static void test_logical(void) {
	test_caseStart("Testing logical operators");

	bool foo[] = {true,false,true,false};
	bool bar[] = {true,true,false,false};
	bool results[4];
	transform(foo,foo + 4,bar,results,logical_and<bool>());
	vector<bool> v(results,results + 4);
	check_content(v,4,true,false,false,false);

	test_caseSucceeded();
}

static void test_adaptors(void) {
	test_caseStart("Testing adaptors");

	{
		pair<int*,int*> ptiter;
		int foo[] = {10,20,30,40,50};
		int bar[] = {8,12,30,40,160};
		ptiter = mismatch(foo,foo + 5,bar,not2(equal_to<int>()));
		test_assertInt(*ptiter.first,30);
		test_assertInt(*ptiter.second,30);
	}

	{
		int numbers[] = {10,20,30,40,50,10};
		test_assertInt(count_if(numbers,numbers + 6,bind1st(equal_to<int>(),10)),2);
	}

	{
		int numbers[] = {10,-20,-30,40,-50};
		test_assertInt(count_if(numbers,numbers + 5,bind2nd(less<int>(),0)),3);
	}

	{
		const char* foo[] = {"10","20","30","40","50"};
		int bar[5];
		transform(foo,foo + 5,bar,ptr_fun(atoi));
		vector<int> v(bar,bar + 5);
		check_content(v,5,10,20,30,40,50);
	}

	{
		vector<string*> numbers;
		numbers.push_back(new string("one"));
		numbers.push_back(new string("two"));
		numbers.push_back(new string("three"));
		numbers.push_back(new string("four"));
		numbers.push_back(new string("five"));
		vector<int> lengths(numbers.size());
		transform(numbers.begin(),numbers.end(),lengths.begin(),mem_fun(&string::length));
		check_content(lengths,5,3,3,5,4,4);
	}

	{
		vector<string> numbers;
		numbers.push_back("one");
		numbers.push_back("two");
		numbers.push_back("three");
		numbers.push_back("four");
		numbers.push_back("five");
		vector<int> lengths(numbers.size());
		transform(numbers.begin(),numbers.end(),lengths.begin(),mem_fun_ref(&string::length));
		check_content(lengths,5,3,3,5,4,4);
	}

	test_caseSucceeded();
}
