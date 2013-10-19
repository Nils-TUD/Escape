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
#include <esc/sllist.h>
#include <esc/test.h>
#include <stdlib.h>

/* forward declarations */
static void test_sllist(void);
static void test_1(void);
static void test_2(void);
static void test_3(void);
static void test_4(void);
static void test_5(void);
static void test_6(void);
static void test_8(void);
static void test_9(void);
static void test_10(void);
static void test_11(void);

/* our test-module */
sTestModule tModSLList = {
	"Single linked list",
	&test_sllist
};

static void test_sllist(void) {
	test_1();
	test_2();
	test_3();
	test_4();
	test_5();
	test_6();
	test_8();
	test_9();
	test_10();
	test_11();
}

static void test_1(void) {
	ulong x = 0x100;
	size_t i,len,oldFree;
	bool res = true;
	sSLList *list;
	test_caseStart("Append & check & remove index 0");
	oldFree = heapspace();

	list = sll_create();
	for(i = 0; i < 20; i++) {
		sll_append(list,(void*)x++);
	}
	x = 0x100;
	for(i = 0; i < 20; i++) {
		if(sll_get(list,i) != (void*)x++) {
			res = false;
			break;
		}
	}
	if(res) {
		for(i = 0; i < 20; i++) {
			sll_removeIndex(list,0);
		}
	}
	test_assertTrue(res);

	len = sll_length(list);
	test_assertSSize(len,0);
	sll_destroy(list,false);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_2(void) {
	ulong x = 0x100;
	size_t i,len,oldFree;
	sSLList *list;
	test_caseStart("Append & remove first (NULL)");
	oldFree = heapspace();

	list = sll_create();
	for(i = 0; i < 2; i++) {
		sll_append(list,(void*)x++);
	}
	for(i = 0; i < 2; i++) {
		sll_removeFirstWith(list,NULL);
	}

	len = sll_length(list);
	test_assertSize(len,0);
	sll_destroy(list,false);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_3(void) {
	ulong x = 0x100;
	size_t i,len,oldFree;
	sSLList *list;
	test_caseStart("Append & remove first (x)");
	oldFree = heapspace();

	list = sll_create();
	for(i = 0; i < 50; i++) {
		sll_append(list,(void*)x++);
	}
	x = 0x100;
	for(i = 0; i < 50; i++) {
		sll_removeFirstWith(list,(void*)x++);
	}

	len = sll_length(list);
	test_assertSize(len,0);
	sll_destroy(list,false);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_4(void) {
	ulong x = 0x100;
	size_t i,oldFree;
	sSLList *list;
	test_caseStart("Create & append & destroy");
	oldFree = heapspace();

	list = sll_create();
	for(i = 0; i < 200; i++) {
		sll_append(list,(void*)x++);
	}
	sll_destroy(list,false);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_5(void) {
	ulong x = 0x100;
	size_t i,oldFree;
	sSLList *list;
	bool res = true;
	test_caseStart("Create & append & insert somewhere & destroy");
	oldFree = heapspace();

	list = sll_create();
	for(i = 0; i < 5; i++) {
		sll_append(list,(void*)x++);
	}
	sll_insert(list,(void*)0x200,3);
	if(sll_get(list,3) != (void*)0x200)
		res = false;
	sll_insert(list,(void*)0x201,2);
	if(sll_get(list,2) != (void*)0x201)
		res = false;
	sll_insert(list,(void*)0x202,1);
	if(sll_get(list,1) != (void*)0x202)
		res = false;
	sll_insert(list,(void*)0x203,0);
	if(sll_get(list,0) != (void*)0x203)
		res = false;
	sll_insert(list,(void*)0x204,7);
	if(sll_get(list,7) != (void*)0x204)
		res = false;
	if(sll_length(list) != 10)
		res = false;
	sll_destroy(list,false);
	test_assertTrue(res);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_6(void) {
	ulong x = 0x100;
	size_t i,oldFree;
	sSLList *list;
	bool res = true;
	test_caseStart("Create & append & set somewhere & destroy");
	oldFree = heapspace();

	list = sll_create();
	for(i = 0; i < 5; i++) {
		sll_append(list,(void*)x++);
	}
	sll_set(list,(void*)0x200,3);
	if(sll_get(list,3) != (void*)0x200)
		res = false;
	sll_set(list,(void*)0x201,2);
	if(sll_get(list,2) != (void*)0x201)
		res = false;
	sll_set(list,(void*)0x202,1);
	if(sll_get(list,1) != (void*)0x202)
		res = false;
	sll_set(list,(void*)0x203,0);
	if(sll_get(list,0) != (void*)0x203)
		res = false;
	sll_set(list,(void*)0x204,4);
	if(sll_get(list,4) != (void*)0x204)
		res = false;
	if(sll_length(list) != 5)
		res = false;
	sll_destroy(list,false);
	test_assertTrue(res);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_8(void) {
	sSLList *list;
	size_t oldFree;
	sSLNode *n;

	test_caseStart("Walking through the list");
	oldFree = heapspace();

	list = sll_create();
	sll_append(list,(void*)0x123);
	sll_append(list,(void*)0x456);
	sll_append(list,(void*)0x789);

	n = sll_begin(list);
	test_assertPtr(n->data,(void*)0x123);
	n = n->next;
	test_assertPtr(n->data,(void*)0x456);
	n = n->next;
	test_assertPtr(n->data,(void*)0x789);
	n = n->next;
	test_assertPtr(n,NULL);

	n = sll_nodeAt(list,2);
	test_assertPtr(n->data,(void*)0x789);
	n = n->next;
	test_assertPtr(n,NULL);

	sll_destroy(list,false);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_9(void) {
	sSLList *list;
	size_t oldFree;

	test_caseStart("Testing sll_indexOf and sll_nodeWith");
	oldFree = heapspace();

	list = sll_create();
	sll_append(list,(void*)0x123);
	sll_append(list,(void*)0x456);
	sll_append(list,(void*)0x789);

	test_assertSSize(sll_indexOf(list,(void*)0x123),0);
	test_assertSSize(sll_indexOf(list,(void*)0x456),1);
	test_assertSSize(sll_indexOf(list,(void*)0x789),2);
	test_assertSSize(sll_indexOf(list,(void*)0x123123),-1);
	test_assertPtr(sll_nodeWith(list,(void*)0x123),sll_nodeAt(list,0));
	test_assertPtr(sll_nodeWith(list,(void*)0x456),sll_nodeAt(list,1));
	test_assertPtr(sll_nodeWith(list,(void*)0x789),sll_nodeAt(list,2));
	test_assertPtr(sll_nodeWith(list,(void*)0x123123),NULL);
	sll_destroy(list,false);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_10(void) {
	sSLList *l1,*l2;
	size_t oldFree;
	test_caseStart("Testing sll_clone");
	oldFree = heapspace();

	l1 = sll_create();
	sll_append(l1,(void*)4);
	sll_append(l1,(void*)3);
	sll_append(l1,(void*)2);
	l2 = sll_clone(l1);
	test_assertSize(sll_length(l2),3);
	test_assertPtr(sll_get(l2,0),(void*)4);
	test_assertPtr(sll_get(l2,1),(void*)3);
	test_assertPtr(sll_get(l2,2),(void*)2);
	sll_destroy(l1,false);
	sll_destroy(l2,false);

	l1 = sll_create();
	l2 = sll_clone(l1);
	test_assertSize(sll_length(l2),0);
	sll_destroy(l2,false);
	sll_destroy(l1,false);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_11(void) {
	sSLList *l1;
	size_t oldFree;
	test_caseStart("Testing sll_removeFirst");
	oldFree = heapspace();

	l1 = sll_create();
	sll_append(l1,(void*)4);
	sll_append(l1,(void*)3);
	sll_append(l1,(void*)2);
	test_assertSize(sll_length(l1),3);
	test_assertPtr(sll_removeFirst(l1),(void*)4);
	test_assertSize(sll_length(l1),2);
	test_assertPtr(sll_removeFirst(l1),(void*)3);
	test_assertSize(sll_length(l1),1);
	test_assertPtr(sll_removeFirst(l1),(void*)2);
	test_assertSize(sll_length(l1),0);
	test_assertPtr(sll_removeFirst(l1),NULL);
	sll_destroy(l1,false);

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}
