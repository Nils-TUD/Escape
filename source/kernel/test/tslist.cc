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

#include <common.h>
#include <col/slist.h>
#include <col/islist.h>
#include <esc/test.h>
#include "testutils.h"

/* a bit of c++ magic to use the same test both for SList and ISList */
template<
	template<class T> class LIST,
	class ITEM,
	class PARAM
>
static void test_slist_tpl(const char *name);

struct MyItem : public SListItem {
	explicit MyItem(int _a) : a(_a) {
	}
	int a;
};
static inline bool operator==(const MyItem &i1,const MyItem &i2) {
	return i1.a == i2.a;
}

struct IMyItem {
	explicit IMyItem(int _a) : a(_a) {
	}
	int a;
};
static inline bool operator==(const IMyItem *i1,const IMyItem &i2) {
	return i1->a == i2.a;
}

static void test_slist();

/* our test-module */
sTestModule tModSList = {
	"SList",
	&test_slist
};

static void test_slist() {
	test_slist_tpl<SList,MyItem,MyItem>("SList");
	test_slist_tpl<ISList,IMyItem,IMyItem*>("ISList");
}

template<
	template<class T> class LIST,
	class ITEM,
	class PARAM
>
static void test_slist_tpl(const char *name) {
	test_caseStart("Testing %s",name);
	checkMemoryBefore(false);

	{
		ITEM e1(1), e2(2), e3(3);
		typename LIST<PARAM>::iterator it;
		LIST<PARAM> l;

		test_assertSize(l.length(), static_cast<size_t>(0));
		test_assertTrue(l.begin() == l.end());

		l.append(&e1);
		l.append(&e2);
		l.append(&e3);
		test_assertSize(l.length(), static_cast<size_t>(3));
		it = l.begin();
		test_assertTrue(*it == e1);
		++it;
		test_assertTrue(*it == e2);
		++it;
		test_assertTrue(*it == e3);
		++it;
		test_assertTrue(it == l.end());

		l.remove(&e2);
		test_assertSize(l.length(), static_cast<size_t>(2));
		it = l.begin();
		test_assertTrue(*it == e1);
		++it;
		test_assertTrue(*it == e3);
		++it;
		test_assertTrue(it == l.end());

		l.remove(&e3);
		test_assertSize(l.length(), static_cast<size_t>(1));
		it = l.begin();
		test_assertTrue(*it == e1);
		++it;
		test_assertTrue(it == l.end());

		l.append(&e3);
		test_assertSize(l.length(), static_cast<size_t>(2));
		it = l.begin();
		test_assertTrue(*it == e1);
		++it;
		test_assertTrue(*it == e3);
		++it;
		test_assertTrue(it == l.end());

		l.remove(&e1);
		l.remove(&e3);
		test_assertSize(l.length(), static_cast<size_t>(0));
		test_assertTrue(l.begin() == l.end());

		l.append(&e2);
		test_assertSize(l.length(), static_cast<size_t>(1));
		it = l.begin();
		test_assertTrue(*it == e2);
		++it;
		test_assertTrue(it == l.end());
	}

	checkMemoryAfter(false);
	test_caseSucceeded();
}
