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
#include <esc/pathtree.h>
#include <assert.h>
#include <stdlib.h>

static void test_pathtree();
static void test_insert();
static void test_clone();
static void test_remove();
static void test_iterator();

/* our test-module */
sTestModule tModPathTree = {
	"PathTree",
	&test_pathtree
};

static void test_pathtree() {
	test_insert();
	test_clone();
	test_remove();
	test_iterator();
}

static void test_insert() {
	size_t before;
	esc::PathTreeItem<void> *i;
	const char *end;
	test_caseStart("Testing insert");

	before = heapspace();
	{
		esc::PathTree<void> mytree;
		test_assertPtr(mytree.find("/"),NULL);
	}
	test_assertSize(heapspace(),before);

	before = heapspace();
	{
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/",(void*)0x11),0);

		i = mytree.find("/",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"");
		test_assertStr(i->getName(),"/");
		test_assertPtr(i->getData(),(void*)0x11);

		i = mytree.find("/../..",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"../..");
		test_assertStr(i->getName(),"/");
		test_assertPtr(i->getData(),(void*)0x11);
	}
	test_assertSize(heapspace(),before);

	before = heapspace();
	{
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/foo",(void*)0x11),0);

		test_assertPtr(mytree.find("/"),NULL);

		i = mytree.find("/foo",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"");
		test_assertStr(i->getName(),"foo");
		test_assertPtr(i->getData(),(void*)0x11);

		i = mytree.find("/foo/bar/test",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"bar/test");
		test_assertPtr(i->getData(),(void*)0x11);
	}
	test_assertSize(heapspace(),before);

	before = heapspace();
	{
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/",(void*)0x11),0);
		test_assertInt(mytree.insert("/foo",(void*)0x12),0);
		test_assertInt(mytree.insert("/foo/bar",(void*)0x13),0);

		mytree.remove("/foo");

		i = mytree.find("/foo",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"foo");
		test_assertStr(i->getName(),"/");
		test_assertPtr(i->getData(),(void*)0x11);
	}
	test_assertSize(heapspace(),before);

	before = heapspace();
	{
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/",(void*)0x11),0);
		test_assertInt(mytree.insert("///foo//",(void*)0x22),0);
		test_assertInt(mytree.insert("/foo/././test/../test",(void*)0x33),0);
		test_assertInt(mytree.insert("/bar",(void*)0x44),0);
		test_assertInt(mytree.insert("/bar/foo/hier/test/../../../../bar/foo/hier/test",(void*)0x55),0);
		test_assertInt(mytree.insert("/bar/foo/hier/test",(void*)0x66),-EEXIST);
		test_assertInt(mytree.insert("/",(void*)0x66),-EEXIST);
		test_assertInt(mytree.insert("/bar",(void*)0x66),-EEXIST);
		test_assertInt(mytree.insert("/bar/foo",(void*)0x66),0);

		i = mytree.find("/",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"");
		test_assertStr(i->getName(),"/");
		test_assertPtr(i->getData(),(void*)0x11);

		i = mytree.find("/test",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"test");
		test_assertPtr(i->getData(),(void*)0x11);

		i = mytree.find("/foo",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"");
		test_assertStr(i->getName(),"foo");
		test_assertPtr(i->getData(),(void*)0x22);

		i = mytree.find("/foo/test",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"");
		test_assertStr(i->getName(),"test");
		test_assertPtr(i->getData(),(void*)0x33);

		i = mytree.find("/bar",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"");
		test_assertStr(i->getName(),"bar");
		test_assertPtr(i->getData(),(void*)0x44);

		i = mytree.find("/bar/foo/hier/test",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"");
		test_assertStr(i->getName(),"test");
		test_assertPtr(i->getData(),(void*)0x55);

		i = mytree.find("/bar///foo/./hier/../hier/test/1//2//3///",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"1//2//3///");
		test_assertPtr(i->getData(),(void*)0x55);

		i = mytree.find("/bar/foo",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"");
		test_assertStr(i->getName(),"foo");
		test_assertPtr(i->getData(),(void*)0x66);
	}
	test_assertSize(heapspace(),before);

	test_caseSucceeded();
}

static void test_clone() {
	size_t before;
	esc::PathTreeItem<void> *i;
	const char *end;
	test_caseStart("Testing clone");

	before = heapspace();
	{
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/",(void*)0x11),0);
		test_assertInt(mytree.insert("///foo//",(void*)0x22),0);
		test_assertInt(mytree.insert("/foo/././test/../test",(void*)0x33),0);
		test_assertInt(mytree.insert("/bar",(void*)0x44),0);
		test_assertInt(mytree.insert("/bar/foo/hier/test/../../../../bar/foo/hier/test",(void*)0x55),0);
		test_assertInt(mytree.insert("/bar/foo",(void*)0x66),0);

		esc::PathTree<void> mytree2;
		test_assertInt(mytree2.replaceWith(mytree),0);

		i = mytree2.find("/");
		test_assertTrue(i != NULL);
		test_assertStr(i->getName(),"/");
		test_assertPtr(i->getData(),(void*)0x11);

		i = mytree2.find("/te",&end);
		test_assertTrue(i != NULL);
		test_assertStr(i->getName(),"/");
		test_assertPtr(i->getData(),(void*)0x11);

		i = mytree2.find("/test",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"test");
		test_assertPtr(i->getData(),(void*)0x11);

		i = mytree2.find("/foo");
		test_assertTrue(i != NULL);
		test_assertStr(i->getName(),"foo");
		test_assertPtr(i->getData(),(void*)0x22);

		i = mytree2.find("/foo/test");
		test_assertTrue(i != NULL);
		test_assertStr(i->getName(),"test");
		test_assertPtr(i->getData(),(void*)0x33);

		i = mytree2.find("/bar");
		test_assertTrue(i != NULL);
		test_assertStr(i->getName(),"bar");
		test_assertPtr(i->getData(),(void*)0x44);

		i = mytree2.find("/bar/foo/hier/test");
		test_assertTrue(i != NULL);
		test_assertStr(i->getName(),"test");
		test_assertPtr(i->getData(),(void*)0x55);

		i = mytree2.find("/bar///foo/./hier/../hier/test/1//2//3///",&end);
		test_assertTrue(i != NULL);
		test_assertStr(end,"1//2//3///");
		test_assertPtr(i->getData(),(void*)0x55);

		i = mytree2.find("/bar/foo");
		test_assertTrue(i != NULL);
		test_assertStr(i->getName(),"foo");
		test_assertPtr(i->getData(),(void*)0x66);

		test_assertInt(mytree.insert("/123",(void*)0x77),0);
		i = mytree2.find("/123");
		test_assertPtr(i->getData(),(void*)0x11);
		i = mytree.find("/123");
		test_assertPtr(i->getData(),(void*)0x77);
	}
	test_assertSize(heapspace(),before);

	{
		before = heapspace();
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/",(void*)0x11),0);

		esc::PathTree<void> mytree2;
		test_assertInt(mytree.replaceWith(mytree2),0);
		test_assertSize(heapspace(),before);
	}

	test_caseSucceeded();
}

static void test_remove() {
	size_t before;
	test_caseStart("Testing remove");

	{
		before = heapspace();
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/",(void*)0x11),0);

		test_assertPtr(mytree.remove("/"),(void*)0x11);
		test_assertPtr(mytree.find("/"),NULL);
		test_assertSize(heapspace(),before);
	}

	{
		before = heapspace();
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/foo",(void*)0x11),0);

		test_assertPtr(mytree.remove("/foo/test"),NULL);
		test_assertTrue(mytree.find("/foo") != NULL);

		test_assertPtr(mytree.remove("/fo"),NULL);
		test_assertTrue(mytree.find("/foo") != NULL);

		test_assertPtr(mytree.remove("/foo"),(void*)0x11);
		test_assertTrue(mytree.find("/foo") == NULL);
		test_assertTrue(mytree.find("/") == NULL);
		test_assertSize(heapspace(),before);
	}

	{
		before = heapspace();;
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/",(void*)0x11),0);
		test_assertInt(mytree.insert("///foo//",(void*)0x22),0);
		test_assertInt(mytree.insert("/foo/././test/../test",(void*)0x33),0);
		test_assertInt(mytree.insert("/bar",(void*)0x44),0);
		test_assertInt(mytree.insert("/bar/foo/hier/test/../../../../bar/foo/hier/test",(void*)0x55),0);
		test_assertInt(mytree.insert("/bar/foo/hier/test",(void*)0x66),-EEXIST);
		test_assertInt(mytree.insert("/",(void*)0x66),-EEXIST);
		test_assertInt(mytree.insert("/bar",(void*)0x66),-EEXIST);
		test_assertInt(mytree.insert("/bar/foo",(void*)0x66),0);

		test_assertPtr(mytree.remove("/bar"),(void*)0x44);
		test_assertPtr(mytree.find("/bar")->getData(),(void*)0x11);
		test_assertPtr(mytree.find("/bar/foo")->getData(),(void*)0x66);

		test_assertPtr(mytree.remove("/bar/foo//"),(void*)0x66);
		test_assertPtr(mytree.find("/bar/foo")->getData(),(void*)0x11);

		test_assertPtr(mytree.remove("/"),(void*)0x11);
		test_assertTrue(mytree.find("/") == NULL);
		test_assertPtr(mytree.find("/foo")->getData(),(void*)0x22);

		test_assertPtr(mytree.remove("/foo/test"),(void*)0x33);
		test_assertPtr(mytree.find("/foo/test")->getData(),(void*)0x22);
		test_assertPtr(mytree.find("/foo")->getData(),(void*)0x22);

		test_assertPtr(mytree.remove("/foo"),(void*)0x22);
		test_assertTrue(mytree.find("/foo") == NULL);

		test_assertPtr(mytree.remove("/bar/foo/hier/test"),(void*)0x55);
		test_assertTrue(mytree.find("/bar/foo/hier/test") == NULL);
		test_assertSize(heapspace(),before);
	}

	test_caseSucceeded();
}

static void test_iterator() {
	size_t before;
	esc::PathTree<void>::iterator it;
	test_caseStart("Testing iterator");

	before = heapspace();
	{
		esc::PathTree<void> mytree;
		test_assertInt(mytree.insert("/",(void*)0x11),0);
		test_assertInt(mytree.insert("/a",(void*)0x12),0);
		test_assertInt(mytree.insert("/a/b",(void*)0x13),0);
		test_assertInt(mytree.insert("/a/a",(void*)0x14),0);
		test_assertInt(mytree.insert("/a/foo",(void*)0x15),0);
		test_assertInt(mytree.insert("/b",(void*)0x16),0);
		test_assertInt(mytree.insert("/b/bar",(void*)0x17),0);
		test_assertInt(mytree.insert("/b/baz",(void*)0x18),0);

		// nodes are stored in opposite order
		it = mytree.begin(mytree.find("/"));
		test_assertPtr(it->getData(),(void*)0x16);
		++it;
		test_assertPtr(it->getData(),(void*)0x12);
		++it;
		test_assertTrue(it == mytree.end());

		it = mytree.begin(mytree.find("/a"));
		test_assertPtr(it->getData(),(void*)0x15);
		++it;
		test_assertPtr(it->getData(),(void*)0x14);
		++it;
		test_assertPtr(it->getData(),(void*)0x13);
		++it;
		test_assertTrue(it == mytree.end());

		it = mytree.begin(mytree.find("/b"));
		test_assertPtr(it->getData(),(void*)0x18);
		++it;
		test_assertPtr(it->getData(),(void*)0x17);
		++it;
		test_assertTrue(it == mytree.end());
	}
	test_assertSize(heapspace(),before);
}
