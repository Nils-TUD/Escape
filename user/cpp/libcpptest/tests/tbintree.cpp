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
#include <impl/map/bintree.h>
#include <utility>

using namespace std;

/* forward declarations */
static void test_bintree(void);
static void test_insert(void);
static void test_order(void);
static void test_copy(void);
static void test_erase(void);
static void test_iterators(void);

/* our test-module */
sTestModule tModBintree = {
	"Binary Tree",
	&test_bintree
};

static void test_bintree(void) {
	test_insert();
	test_order();
	test_copy();
	test_erase();
	test_iterators();
}

static void test_insert(void) {
	u32 before,after;
	test_caseStart("Testing insert");

	bintree<int,int>::iterator it;

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(1,1);
		t.insert(2,3);
		t.insert(0,1);
		test_assertUInt(t.size(),3);

		it = t.find(2);
		test_assertTrue(*it == make_pair(2,3));
		it = t.find(1);
		test_assertTrue(*it == make_pair(1,1));
		it = t.find(0);
		test_assertTrue(*it == make_pair(0,1));
		it = t.find(4);
		test_assertTrue(it == t.end());
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(0,4);
		test_assertUInt(t.size(),1);

		it = t.find(0);
		test_assertTrue(*it == make_pair(0,4));
		it = t.find(4);
		test_assertTrue(it == t.end());
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		for(int i = 0; i < 20; i++)
			t.insert(i,i);
		test_assertUInt(t.size(),20);

		for(int i = 19; i >= 0; i--) {
			it = t.find(i);
			test_assertTrue(*it == make_pair(i,i));
		}

		it = t.find(20);
		test_assertTrue(it == t.end());
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		test_assertUInt(t.size(),0);
		it = t.find(4);
		test_assertTrue(it == t.end());
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(t.begin(),3,3);
		t.insert(4,4);
		t.insert(1,1);
		t.insert(2,2);
		it = t.find(4);
		t.insert(it,5,5);
		it = t.find(4);
		t.insert(it,0,0);
		it = t.find(2);
		t.insert(it,2,3);

		test_assertUInt(t.size(),6);
		it = t.find(0);
		test_assertTrue(*it == make_pair(0,0));
		it = t.find(1);
		test_assertTrue(*it == make_pair(1,1));
		it = t.find(2);
		test_assertTrue(*it == make_pair(2,3));
		it = t.find(3);
		test_assertTrue(*it == make_pair(3,3));
		it = t.find(4);
		test_assertTrue(*it == make_pair(4,4));
		it = t.find(5);
		test_assertTrue(*it == make_pair(5,5));
	}
	after = heapspace();
	test_assertUInt(after,before);

	test_caseSucceded();
}

static void test_order(void) {
	test_caseStart("Testing copy");

	int ints[] = {1,3,4,7,8,12,14,16};
	bintree<int,int> t;
	t.insert(16,16);
	t.insert(14,14);
	t.insert(1,1);
	t.insert(8,8);
	t.insert(4,4);
	t.insert(7,7);
	t.insert(12,12);
	t.insert(3,3);

	int i = 0;
	for(bintree<int,int>::iterator it = t.begin(); it != t.end(); ++it)
		test_assertInt((*it).first,ints[i++]);

	test_caseSucceded();
}

static void test_copy(void) {
	u32 before,after;
	test_caseStart("Testing copy");

	bintree<int,int>::iterator it;

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(1,1);
		t.insert(2,3);
		t.insert(0,1);
		bintree<int,int> cpy(t);

		it = cpy.find(2);
		test_assertTrue(*it == make_pair(2,3));
		it = cpy.find(1);
		test_assertTrue(*it == make_pair(1,1));
		it = cpy.find(0);
		test_assertTrue(*it == make_pair(0,1));
		it = cpy.find(4);
		test_assertTrue(it == cpy.end());
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(1,1);
		t.insert(2,3);
		t.insert(0,1);
		bintree<int,int> t2;
		t2.insert(4,5);
		t2.insert(5,6);
		t = t2;

		it = t.find(4);
		test_assertTrue(*it == make_pair(4,5));
		it = t.find(5);
		test_assertTrue(*it == make_pair(5,6));
		it = t.find(0);
		test_assertTrue(it == t.end());
		it = t.find(1);
		test_assertTrue(it == t.end());
	}
	after = heapspace();
	test_assertUInt(after,before);

	test_caseSucceded();
}

static void test_erase(void) {
	u32 before,after;
	test_caseStart("Testing erase");

	bintree<int,int>::iterator it;

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(1,1);
		t.insert(2,3);
		t.insert(0,1);

		t.erase(2);
		it = t.find(2);
		test_assertTrue(it == t.end());
		it = t.find(1);
		test_assertTrue(*it == make_pair(1,1));
		it = t.find(0);
		test_assertTrue(*it == make_pair(0,1));

		t.erase(1);
		it = t.find(2);
		test_assertTrue(it == t.end());
		it = t.find(1);
		test_assertTrue(it == t.end());
		it = t.find(0);
		test_assertTrue(*it == make_pair(0,1));

		t.erase(0);
		it = t.find(2);
		test_assertTrue(it == t.end());
		it = t.find(1);
		test_assertTrue(it == t.end());
		it = t.find(0);
		test_assertTrue(it == t.end());
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(1,1);
		t.insert(2,3);
		t.insert(0,1);

		t.erase(4);
		t.erase(0);
		t.erase(1);
		t.erase(2);
		test_assertUInt(t.size(),0);
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		for(int i = 0; i < 20; ++i)
			t.insert(i,i);

		for(int i = 0; i < 20; ++i)
			t.erase(i);
		test_assertUInt(t.size(),0);
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		for(int i = 0; i < 20; ++i)
			t.insert(i,i);

		for(int i = 19; i >= 0; --i)
			t.erase(i);
		test_assertUInt(t.size(),0);
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		t.erase(0);
		it = t.find(0);
		test_assertTrue(it == t.end());
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(1,1);
		t.insert(2,3);
		t.insert(0,1);

		it = t.begin();
		t.erase(it);
		it = t.begin();
		advance(it,1);
		t.erase(it);

		it = t.find(1);
		test_assertTrue(*it == make_pair(1,1));
		test_assertUInt(t.size(),1);
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(1,1);
		t.insert(2,3);
		t.insert(0,1);

		t.erase(t.begin(),t.end());
		test_assertUInt(t.size(),0);
	}
	after = heapspace();
	test_assertUInt(after,before);

	before = heapspace();
	{
		bintree<int,int> t;
		t.insert(1,1);
		t.insert(2,3);
		t.insert(0,1);

		it = t.begin();
		advance(it,1);
		t.erase(it,t.end());

		it = t.find(2);
		test_assertTrue(*it == make_pair(2,3));
		test_assertUInt(t.size(),1);
	}
	after = heapspace();
	test_assertUInt(after,before);

	test_caseSucceded();
}

static void test_iterators(void) {
	test_caseStart("Testing iterators");

	bintree<int,int> t;
	for(int i = 0; i < 8; i++)
		t.insert(i,i + 1);

	int i = 0;
	for(bintree<int,int>::iterator it = t.begin(); it != t.end(); ++it) {
		test_assertInt(it->first,i);
		test_assertInt(it->second,i + 1);
		i++;
	}

	i = 7;
	for(bintree<int,int>::reverse_iterator it = t.rbegin(); it != t.rend(); ++it) {
		test_assertInt(it->first,i);
		test_assertInt(it->second,i + 1);
		i--;
	}

	test_caseSucceded();
}
