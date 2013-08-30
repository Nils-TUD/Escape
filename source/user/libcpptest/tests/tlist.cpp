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
#include <list>

using namespace std;

/* forward declarations */
static void test_list(void);
static void test_constr(void);
static void test_assign(void);
static void test_iterators(void);
static void test_insert(void);
static void test_erase(void);
static void test_splice(void);
static void test_resize(void);
static void test_remove(void);
static void test_unique(void);
static void test_sort(void);
static void test_merge(void);
static void test_reverse(void);

static void check_content(const list<int> &l,size_t count,...) {
	va_list ap;
	va_start(ap,count);
	test_assertSize(l.size(),count);
	for(auto it = l.begin(); it != l.end(); it++)
		test_assertInt(*it,va_arg(ap,int));
	va_end(ap);
}

/* our test-module */
sTestModule tModList = {
	"list",
	&test_list
};

static void test_list(void) {
	test_constr();
	test_assign();
	test_iterators();
	test_insert();
	test_erase();
	test_splice();
	test_resize();
	test_remove();
	test_unique();
	test_sort();
	test_merge();
	test_reverse();
}

static void test_constr(void) {
	test_caseStart("Testing constructors");

	list<int> v;
	test_assertUInt(v.size(),0);

	list<int> v2((size_t)5,4);
	check_content(v2,5,4,4,4,4,4);

	list<int> v3(v2.begin(),v2.end());
	check_content(v3,5,4,4,4,4,4);

	list<int> v4(v3);
	check_content(v4,5,4,4,4,4,4);

	test_caseSucceeded();
}

static void test_assign(void) {
	test_caseStart("Testing assign");

	list<int> v1(5u,3);
	list<int> v2 = v1;
	test_assertSize(v1.size(),v2.size());
	check_content(v2,5,3,3,3,3,3);

	list<int> v3;
	v3.assign(v1.begin() + 1,v1.end());
	test_assertSize(v3.size(),v1.size() - 1);
	check_content(v3,4,3,3,3,3);

	list<int> v4;
	v4.assign(3u,3);
	test_assertSize(v4.size(),3);
	check_content(v4,3,3,3,3);

	v4.front() += 1;
	v4.back() -= 1;
	check_content(v4,3,4,3,2);

	test_caseSucceeded();
}

static void test_iterators(void) {
	test_caseStart("Testing iterators");

	{
		list<int> v1;
		test_assertTrue(v1.begin() == v1.end());
		test_assertTrue(v1.rbegin() == v1.rend());
		for(auto it = v1.begin(); it != v1.end(); it++)
			;
		for(auto it = v1.rbegin(); it != v1.rend(); it++)
			;
	}

	{
		list<int> v1;
		v1.push_back(1);
		v1.push_back(2);
		v1.push_back(3);
		v1.push_back(4);
		int i = 1;
		for(auto it = v1.begin(); it != v1.end(); it++)
			test_assertInt(*it,i++);
		i = 4;
		for(auto it = v1.rbegin(); it != v1.rend(); it++)
			test_assertInt(*it,i--);
	}

	test_caseSucceeded();
}

static void test_insert(void) {
	test_caseStart("Testing insert");

	size_t before = heapspace();

	{
		list<int>::iterator it;
		list<int> v1;
		it = v1.insert(v1.end(),4);
		test_assertInt(*it,4);
		check_content(v1,1,4);
		it = v1.insert(v1.end(),5);
		test_assertInt(*it,5);
		check_content(v1,2,4,5);
		it = v1.insert(v1.begin(),1);
		test_assertInt(*it,1);
		check_content(v1,3,1,4,5);
		it = v1.insert(v1.begin() + 1,2);
		test_assertInt(*it,2);
		check_content(v1,4,1,2,4,5);
		it = v1.insert(v1.begin() + 2,3);
		test_assertInt(*it,3);
		check_content(v1,5,1,2,3,4,5);

		v1.insert(v1.begin() + 1,3u,8);
		check_content(v1,8,1,8,8,8,2,3,4,5);

		list<int> v2(2u,6);
		v1.insert(v1.begin() + 1,v2.begin(),v2.end());
		check_content(v1,10,1,6,6,8,8,8,2,3,4,5);
	}

	size_t after = heapspace();
	test_assertSize(after,before);

	test_caseSucceeded();
}

static void test_erase(void) {
	test_caseStart("Testing erase");

	size_t before = heapspace();

	{
		list<int>::iterator it;
		list<int> v1;
		v1.push_back(1);
		v1.push_back(2);
		v1.push_back(3);
		v1.push_back(4);
		v1.push_back(5);

		it = v1.erase(--v1.end());
		test_assertTrue(it == v1.end());
		check_content(v1,4,1,2,3,4);
		it = v1.erase(v1.begin() + 1);
		test_assertInt(*it,3);
		check_content(v1,3,1,3,4);
		it = v1.erase(v1.begin() + 1,v1.end());
		test_assertTrue(it == v1.end());
		check_content(v1,1,1);
		it = v1.erase(v1.begin(),v1.end());
		test_assertTrue(it == v1.end());
		check_content(v1,0);
	}

	size_t after = heapspace();
	test_assertSize(after,before);

	test_caseSucceeded();
}

static void test_splice(void) {
	test_caseStart("Testing splice");

	size_t before = heapspace();

	{
		list<int> mylist1,mylist2;
		list<int>::iterator it;
		for(int i = 1; i <= 4; i++)
			mylist1.push_back(i);
		for(int i = 1; i <= 3; i++)
			mylist2.push_back(i * 10);

		it = mylist1.begin();
		++it;

		mylist1.splice(it,mylist2);
		check_content(mylist1,7,1,10,20,30,2,3,4);
		check_content(mylist2,0);

		mylist2.splice(mylist2.begin(),mylist1,it);
		check_content(mylist1,6,1,10,20,30,3,4);
		check_content(mylist2,1,2);

		it = mylist1.begin();
		it += 3;

		mylist1.splice(mylist1.begin(),mylist1,it,mylist1.end());
		check_content(mylist1,6,30,3,4,1,10,20);

		list<int> l1,l2;
		l1.splice(l1.begin(),l1,l1.begin(),l1.end());
		l1.splice(l1.begin(),l2,l2.begin(),l2.end());
	}

	size_t after = heapspace();
	test_assertSize(after,before);

	test_caseSucceeded();
}

static void test_resize(void) {
	test_caseStart("Testing resize");

	size_t before = heapspace();

	{
		list<int> l;
		for(size_t i = 1; i < 10; i++)
			l.push_back(i);

		l.resize(5);
		check_content(l,5,1,2,3,4,5);

		l.resize(8,100);
		check_content(l,8,1,2,3,4,5,100,100,100);

		l.resize(12);
		check_content(l,12,1,2,3,4,5,100,100,100,0,0,0,0);
	}

	size_t after = heapspace();
	test_assertSize(after,before);

	test_caseSucceeded();
}

static bool removePred(int v) {
	return v >= 3;
}

static void test_remove(void) {
	test_caseStart("Testing remove");

	size_t before = heapspace();

	{
		list<int> l;
		l.push_back(1);
		l.push_back(2);
		l.push_back(3);
		l.push_back(3);
		l.push_back(2);
		l.push_back(8);
		l.push_back(8);
		l.push_back(1);
		l.push_back(4);

		l.remove(2);
		check_content(l,7,1,3,3,8,8,1,4);

		l.remove(4);
		check_content(l,6,1,3,3,8,8,1);

		l.remove(12);
		check_content(l,6,1,3,3,8,8,1);

		l.remove_if(&removePred);
		check_content(l,2,1,1);
	}

	size_t after = heapspace();
	test_assertSize(after,before);

	test_caseSucceeded();
}

static bool isSame(int a,int b) {
	return (a - b < 0 ? b - a : a - b) <= 1;
}

static void test_unique(void) {
	test_caseStart("Testing unique");

	size_t before = heapspace();

	{
		list<int> l;
		l.push_back(1);
		l.push_back(2);
		l.push_back(3);
		l.push_back(3);
		l.push_back(2);
		l.push_back(8);
		l.push_back(8);
		l.push_back(1);
		l.push_back(4);

		l.unique();
		check_content(l,7,1,2,3,2,8,1,4);

		l.unique(isSame);
		check_content(l,5,1,3,8,1,4);
	}

	size_t after = heapspace();
	test_assertSize(after,before);

	test_caseSucceeded();
}

static void test_sort(void) {
	test_caseStart("Testing sort");

	size_t before = heapspace();

	{
		list<int> l;
		l.push_back(14);
		l.push_back(21);
		l.push_back(2);
		l.push_back(44);
		l.push_back(1);
		l.push_back(5);
		l.push_back(2);
		l.push_back(16);
		l.sort();
		check_content(l,8,1,2,2,5,14,16,21,44);

		list<int> l2;
		l2.push_back(5);
		l2.push_back(4);
		l2.push_back(3);
		l2.push_back(2);
		l2.push_back(1);
		l2.sort();
		check_content(l2,5,1,2,3,4,5);

		list<int> l3;
		l3.sort();
	}

	size_t after = heapspace();
	test_assertSize(after,before);

	test_caseSucceeded();
}

static void test_merge(void) {
	test_caseStart("Testing merge");

	size_t before = heapspace();

	{
		list<int> l;
		l.push_back(1);
		l.push_back(2);
		l.push_back(2);
		l.push_back(5);
		l.push_back(14);
		l.push_back(16);
		l.push_back(21);
		l.push_back(44);
		list<int> l2;
		l2.push_back(2);
		l2.push_back(4);
		l2.push_back(6);
		l2.push_back(66);
		l.merge(l2);
		check_content(l,12,1,2,2,2,4,5,6,14,16,21,44,66);
	}

	size_t after = heapspace();
	test_assertSize(after,before);

	test_caseSucceeded();
}

static void test_reverse(void) {
	test_caseStart("Testing reverse");

	size_t before = heapspace();

	{
		list<int> l;
		l.push_back(1);
		l.push_back(2);
		l.push_back(2);
		l.push_back(5);
		l.push_back(14);
		l.push_back(16);
		l.push_back(21);
		l.push_back(44);
		l.reverse();
		check_content(l,8,44,21,16,14,5,2,2,1);

		list<int> l2;
		l2.push_back(1);
		l2.reverse();
		check_content(l2,1,1);

		list<int> l3;
		l3.push_back(1);
		l3.push_back(2);
		l3.push_back(3);
		l3.reverse();
		check_content(l3,3,3,2,1);

		list<int> l4;
		l4.swap(l3);
		l4.reverse();
		l3 == l4;
		check_content(l4,3,1,2,3);
	}

	size_t after = heapspace();
	test_assertSize(after,before);

	test_caseSucceeded();
}
