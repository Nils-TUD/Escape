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
#include <sys/mem/kheap.h>
#include <esc/test.h>
#include <esc/hashmap.h>
#include <sys/video.h>
#include "thashmap.h"
#include "testutils.h"

#define TEST_MAP_SIZE	16

static void test_hashmap(void);
static void test_basic(void);
static void test_remove(void);
static void test_iterator(void);
static ulong test_getkey(const void *data) {
	return (ulong)data;
}

/* our test-module */
sTestModule tModHashMap = {
	"HashMap",
	&test_hashmap
};

static void test_hashmap(void) {
	test_basic();
	test_remove();
	test_iterator();
}

static void test_basic(void) {
	sSLList *testmap[TEST_MAP_SIZE] = {NULL};
	sHashMap *m;
	test_caseStart("Testing basic functionality");

	checkMemoryBefore(false);
	m = hm_create(testmap,TEST_MAP_SIZE,test_getkey);
	hm_add(m,(void*)1);
	hm_add(m,(void*)2);
	hm_add(m,(void*)3);
	hm_add(m,(void*)16);
	hm_add(m,(void*)17);
	hm_add(m,(void*)19);

	test_assertSize(hm_getCount(m),6);
	test_assertPtr(hm_get(m,1),(void*)1);
	test_assertPtr(hm_get(m,2),(void*)2);
	test_assertPtr(hm_get(m,3),(void*)3);
	test_assertPtr(hm_get(m,16),(void*)16);
	test_assertPtr(hm_get(m,17),(void*)17);
	test_assertPtr(hm_get(m,19),(void*)19);
	test_assertPtr(hm_get(m,0),NULL);
	test_assertPtr(hm_get(m,27),NULL);

	hm_destroy(m);
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_remove(void) {
	sSLList *testmap[TEST_MAP_SIZE] = {NULL};
	sHashMap *m;
	test_caseStart("Testing remove");

	checkMemoryBefore(false);
	m = hm_create(testmap,TEST_MAP_SIZE,test_getkey);
	hm_add(m,(void*)6);
	hm_add(m,(void*)5);
	hm_add(m,(void*)1);
	hm_add(m,(void*)88);
	hm_add(m,(void*)16);
	hm_add(m,(void*)3);

	test_assertSize(hm_getCount(m),6);
	hm_remove(m,(void*)3);
	test_assertPtr(hm_get(m,3),NULL);
	test_assertSize(hm_getCount(m),5);
	hm_remove(m,(void*)1);
	test_assertPtr(hm_get(m,1),NULL);
	test_assertSize(hm_getCount(m),4);
	hm_remove(m,(void*)88);
	test_assertPtr(hm_get(m,88),NULL);
	test_assertSize(hm_getCount(m),3);
	hm_remove(m,(void*)5);
	test_assertPtr(hm_get(m,5),NULL);
	test_assertSize(hm_getCount(m),2);
	hm_remove(m,(void*)16);
	test_assertPtr(hm_get(m,16),NULL);
	test_assertSize(hm_getCount(m),1);
	hm_remove(m,(void*)6);
	test_assertPtr(hm_get(m,6),NULL);
	test_assertSize(hm_getCount(m),0);

	hm_destroy(m);
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_iterator(void) {
	sSLList *testmap[TEST_MAP_SIZE] = {NULL};
	sHashMap *m;
	size_t j;
	ulong i,elems[] = {44,12,18,34,1,109};
	test_caseStart("Testing map iterator");

	checkMemoryBefore(false);
	m = hm_create(testmap,TEST_MAP_SIZE,test_getkey);
	for(i = 0; i < ARRAY_SIZE(elems); i++)
		hm_add(m,(void*)elems[i]);
	test_assertSize(hm_getCount(m),ARRAY_SIZE(elems));

	hm_remove(m,(void*)elems[2]);

	j = 0;
	for(hm_begin(m); (i = (ulong)hm_next(m)); ) {
		tprintf("%d: %u\n",j,i);
		test_assertTrue(i == elems[0] || i == elems[1] ||
				i == elems[3] || i == elems[4] || i == elems[5]);
		j++;
	}
	test_assertSize(hm_getCount(m),j);

	hm_destroy(m);
	checkMemoryAfter(false);

	test_caseSucceeded();
}
