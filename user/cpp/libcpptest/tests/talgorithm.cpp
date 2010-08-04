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
#include <algorithm>
#include <list>
#include <vector>

using namespace esc;

/* forward declarations */
static void test_algo(void);
static void test_find(void);
static void test_count(void);
static void test_transform(void);
static void test_replace(void);
static void test_fill(void);
static void test_generate(void);
static void test_remove(void);
static void test_reverse(void);
static void test_binsearch(void);
static void test_merge(void);
static void test_includes(void);
static void test_minmax(void);
static void test_lexcompare(void);

static void check_content(const list<int> &l,size_t count,...) {
	va_list ap;
	va_start(ap,count);
	test_assertUInt(l.size(),count);
	for(list<int>::const_iterator it = l.begin(); it != l.end(); it++)
		test_assertInt(*it,va_arg(ap,int));
	va_end(ap);
}

/* our test-module */
sTestModule tModAlgo = {
	"Algorithm",
	&test_algo
};

static void test_algo(void) {
	test_find();
	test_count();
	test_transform();
	test_replace();
	test_fill();
	test_generate();
	test_remove();
	test_reverse();
	test_binsearch();
	test_merge();
	test_includes();
	test_minmax();
	test_lexcompare();
}

static void test_find(void) {
	test_caseStart("Testing find");

	int ints[] = {1,4,5,2,3,1,2,2,6,7,8};
	list<int> l(ints,ints + ARRAY_SIZE(ints));

	test_assertInt(*find(l.begin(),l.end(),5),5);
	test_assertTrue(find(l.begin(),l.end(),1) == l.begin());
	test_assertTrue(find(l.begin(),l.end(),12) == l.end());

	int r1ints[] = {1,4,5};
	list<int> r1(r1ints,r1ints + ARRAY_SIZE(r1ints));
	test_assertTrue(search(l.begin(),l.end(),r1.begin(),r1.end()) == l.begin());
	test_assertTrue(find_end(l.begin(),l.end(),r1.begin(),r1.end()) == l.begin());
	test_assertTrue(find_first_of(l.begin(),l.end(),r1.begin(),r1.end()) == l.begin());

	int r2ints[] = {2,2};
	list<int> r2(r2ints,r2ints + ARRAY_SIZE(r2ints));
	test_assertTrue(search(l.begin(),l.end(),r2.begin(),r2.end()) == l.begin() + 6);
	test_assertTrue(find_end(l.begin(),l.end(),r2.begin(),r2.end()) == l.begin() + 6);
	test_assertTrue(find_first_of(l.begin(),l.end(),r2.begin(),r2.end()) == l.begin() + 3);

	int r3ints[] = {2};
	list<int> r3(r3ints,r3ints + ARRAY_SIZE(r3ints));
	test_assertTrue(search(l.begin(),l.end(),r3.begin(),r3.end()) == l.begin() + 3);
	test_assertTrue(find_end(l.begin(),l.end(),r3.begin(),r3.end()) == l.begin() + 7);
	test_assertTrue(find_first_of(l.begin(),l.end(),r3.begin(),r3.end()) == l.begin() + 3);

	int r4ints[] = {5,2,4};
	list<int> r4(r4ints,r4ints + ARRAY_SIZE(r4ints));
	test_assertTrue(search(l.begin(),l.end(),r4.begin(),r4.end()) == l.end());
	test_assertTrue(find_end(l.begin(),l.end(),r4.begin(),r4.end()) == l.end());
	test_assertTrue(find_first_of(l.begin(),l.end(),r4.begin(),r4.end()) == l.begin() + 1);

	int r5ints[] = {9,10,11};
	list<int> r5(r5ints,r5ints + ARRAY_SIZE(r5ints));
	test_assertTrue(search(l.begin(),l.end(),r5.begin(),r5.end()) == l.end());
	test_assertTrue(find_first_of(l.begin(),l.end(),r5.begin(),r5.end()) == l.end());

	test_caseSucceded();
}

static void test_count(void) {
	test_caseStart("Testing count");

	int ints[] = {1,4,5,2,3,1,2,2,6,7,8};
	list<int> l(ints,ints + ARRAY_SIZE(ints));

	test_assertInt(count(l.begin(),l.end(),1),2);
	test_assertInt(count(l.begin(),l.end(),2),3);
	test_assertInt(count(l.begin(),l.end(),6),1);
	test_assertInt(count(l.begin(),l.end(),12),0);

	test_caseSucceded();
}

static int op_increase(int i) {
	return ++i;
}
static int op_sum(int i,int j) {
	return i + j;
}

static void test_transform(void) {
	test_caseStart("Testing transform");

	list<int> first;
	list<int> second;
	list<int>::iterator it;

	for(int i = 1; i < 6; i++)
		first.push_back(i * 10);

	second.resize(first.size());
	transform(first.begin(),first.end(),second.begin(),op_increase);
	check_content(second,5,11,21,31,41,51);

	transform(first.begin(),first.end(),second.begin(),first.begin(),op_sum);
	check_content(first,5,21,41,61,81,101);

	test_caseSucceded();
}

static bool greater(int a) {
	return a > 10;
}

static void test_replace(void) {
	test_caseStart("Testing replace");

	int ints[] = {10,20,30,30,20,10,10,20};
	list<int> l(ints,ints + ARRAY_SIZE(ints));

	replace(l.begin(),l.end(),20,99);
	check_content(l,8,10,99,30,30,99,10,10,99);

	replace_if(l.begin(),l.end(),greater,12);
	check_content(l,8,10,12,12,12,12,10,10,12);

	test_caseSucceded();
}

static void test_fill(void) {
	test_caseStart("Testing fill");

	list<int> l(8);

	fill(l.begin(),l.begin() + 4,5);
	check_content(l,8,5,5,5,5,0,0,0,0);

	fill(l.begin() + 3,l.end() - 2,8);
	check_content(l,8,5,5,5,8,8,8,0,0);

	test_caseSucceded();
}

template<int val>
static int genValue() {
	return val;
}

static void test_generate(void) {
	test_caseStart("Testing generate");

	list<int> l(8);

	generate(l.begin(),l.begin() + 4,genValue<3>);
	check_content(l,8,3,3,3,3,0,0,0,0);

	generate_n(l.begin() + 3,3,genValue<8>);
	check_content(l,8,3,3,3,8,8,8,0,0);

	test_caseSucceded();
}

static void test_remove(void) {
	test_caseStart("Testing remove");

	int ints[] = {10,20,30,30,20,10,10,20};
	list<int> l(ints,ints + ARRAY_SIZE(ints));

	remove(l.begin(),l.end(),20);
	// use resize here to remove the elements at the end (remove leaves them at the end)
	l.resize(5);
	check_content(l,5,10,30,30,10,10);

	remove(l.begin(),l.end(),10);
	l.resize(2);
	check_content(l,2,30,30);

	remove(l.begin(),l.end(),30);
	l.resize(0);
	check_content(l,0);

	test_caseSucceded();
}

static void test_reverse(void) {
	test_caseStart("Testing reverse");

	int ints[] = {1,2,2,5,14,16,21,44};
	list<int> l(ints,ints + ARRAY_SIZE(ints));
	reverse(l.begin(),l.end());
	check_content(l,8,44,21,16,14,5,2,2,1);

	int ints2[] = {1};
	list<int> l2(ints2,ints2 + ARRAY_SIZE(ints2));
	reverse(l2.begin(),l2.end());
	check_content(l2,1,1);

	int ints3[] = {1,2,3};
	list<int> l3(ints3,ints3 + ARRAY_SIZE(ints3));
	reverse(l3.begin(),l3.end());
	check_content(l3,3,3,2,1);

	list<int> l4;
	l4.swap(l3);
	reverse(l4.begin(),l4.end());
	check_content(l4,3,1,2,3);

	int ints5[] = {1,2,3,4,5,6,7,8,9};
	list<int> l5;
	l5.resize(9);
	reverse_copy(ints5,ints5 + ARRAY_SIZE(ints5),l5.begin());
	check_content(l5,9,9,8,7,6,5,4,3,2,1);

	test_caseSucceded();
}

static void test_binsearch(void) {
	test_caseStart("Testing binsearch");

	int ints[] = {10,20,30,30,20,10,10,20};
	vector<int> l(ints,ints + ARRAY_SIZE(ints));
	vector<int>::iterator low,up;

	sort(l.begin(),l.end());

	low = lower_bound(l.begin(),l.end(),20);
	test_assertTrue(low == l.begin() + 3);
	up = upper_bound(l.begin(),l.end(),20);
	test_assertTrue(up == l.begin() + 6);

	test_assertTrue(binary_search(l.begin(),l.end(),10));
	test_assertTrue(binary_search(l.begin(),l.end(),20));
	test_assertTrue(binary_search(l.begin(),l.end(),30));
	test_assertFalse(binary_search(l.begin(),l.end(),40));

	test_caseSucceded();
}

static void test_merge(void) {
	test_caseStart("Testing merge");

	int first[] = {5,10,15,20,25};
	int second[] = {50,40,30,20,10};
	list<int> l(10);
	list<int>::iterator it;

	sort(first,first + ARRAY_SIZE(first));
	sort(second,second + ARRAY_SIZE(second));
	merge(first,first + 5,second,second + 5,l.begin());
	check_content(l,10,5,10,10,15,20,20,25,30,40,50);

	test_caseSucceded();
}

static void test_includes(void) {
	test_caseStart("Testing includes");

	int i1[] = {1,2,3,4,5,6,7};
	list<int> l1(i1,i1 + ARRAY_SIZE(i1));
	int i2[] = {3,4,5};
	list<int> l2(i2,i2 + ARRAY_SIZE(i2));

	test_assertTrue(includes(l1.begin(),l1.end(),l2.begin(),l2.end()));
	test_assertTrue(includes(l1.begin(),l1.end(),l1.begin(),l1.end()));
	test_assertTrue(includes(l2.begin(),l2.end(),l2.begin(),l2.end()));
	test_assertFalse(includes(l2.begin(),l2.end(),l1.begin(),l1.end()));

	test_caseSucceded();
}

static void test_minmax(void) {
	test_caseStart("Testing minmax");

	int i1[] = {6,7,3,4,2,1,5};
	list<int> l1(i1,i1 + ARRAY_SIZE(i1));
	int i2[] = {4,3,5};
	list<int> l2(i2,i2 + ARRAY_SIZE(i2));

	test_assertTrue(min_element(l1.begin(),l1.end()) == l1.begin() + 5);
	test_assertTrue(max_element(l1.begin(),l1.end()) == l1.begin() + 1);
	test_assertTrue(min_element(l2.begin(),l2.end()) == l2.begin() + 1);
	test_assertTrue(max_element(l2.begin(),l2.end()) == l2.begin() + 2);
	test_assertTrue(max_element(l2.begin(),l2.begin()) == l2.begin());
	test_assertTrue(max_element(l2.end(),l2.end()) == l2.end());
	test_assertTrue(min_element(l2.begin(),l2.begin()) == l2.begin());
	test_assertTrue(min_element(l2.end(),l2.end()) == l2.end());

	test_caseSucceded();
}

static void test_lexcompare(void) {
	test_caseStart("Testing lexicographic_compare");

	char first[] = "Apple";
	char second[] = "apartment";

	test_assertTrue(lexicographical_compare(first,first + 5,second,second + 9));
	test_assertFalse(lexicographical_compare(second,second + 9,first,first + 5));

	test_caseSucceded();
}
