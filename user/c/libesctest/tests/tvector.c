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
#include <esc/util/vector.h>
#include <test.h>
#include <stdlib.h>
#include "tstring.h"

/* forward declarations */
static void test_vector(void);
static void test_create(void);
static void test_insert(void);
static void test_iterator(void);
static void test_sort(void);
static void test_remove(void);

/* our test-module */
sTestModule tModVector = {
	"Vector",
	&test_vector
};

static void test_vector(void) {
	test_create();
	test_insert();
	test_iterator();
	test_sort();
	test_remove();
}

static void test_create(void) {
	u32 before;
	test_caseStart("Testing create & destroy");

	before = heapspace();
	sVector *v = vec_create(sizeof(u32));
	test_assertUInt(v->count,0);
	vec_destroy(v,false);
	test_assertUInt(heapspace(),before);

	before = heapspace();
	sVector *v1 = vec_create(sizeof(u32));
	sVector *v2 = vec_copy(v1);
	test_assertUInt(v1->count,0);
	test_assertUInt(v2->count,0);
	vec_destroy(v1,false);
	vec_destroy(v2,false);
	test_assertUInt(heapspace(),before);

	test_caseSucceded();
}

static void test_insert(void) {
	u32 i;
	test_caseStart("Testing insert & append");

	sVector *v = vec_create(sizeof(u32));
	vec_addInt(v,12);
	vec_addInt(v,13);
	i = 14;
	vec_add(v,&i);
	test_assertUInt(v->count,3);
	test_assertUInt(vec_findInt(v,12),0);
	test_assertUInt(vec_findInt(v,13),1);
	test_assertUInt(vec_find(v,&i),2);
	vec_destroy(v,false);

	v = vec_create(sizeof(u32));
	vec_insertInt(v,0,1);
	vec_insertInt(v,1,2);
	vec_insertInt(v,2,3);
	test_assertUInt(v->count,3);
	test_assertUInt(vec_findInt(v,1),0);
	test_assertUInt(vec_findInt(v,2),1);
	test_assertUInt(vec_findInt(v,3),2);
	vec_destroy(v,false);

	v = vec_create(sizeof(u32));
	vec_addInt(v,1);
	vec_addInt(v,2);
	vec_insertInt(v,0,0);
	vec_insertInt(v,2,4);
	vec_insertInt(v,3,5);
	test_assertUInt(v->count,5);
	test_assertUInt(vec_findInt(v,0),0);
	test_assertUInt(vec_findInt(v,1),1);
	test_assertUInt(vec_findInt(v,2),4);
	test_assertUInt(vec_findInt(v,4),2);
	test_assertUInt(vec_findInt(v,5),3);
	vec_destroy(v,false);

	test_caseSucceded();
}

static void test_iterator(void) {
	test_caseStart("Testing iterator");

	sVector *v = vec_create(sizeof(u32));
	vec_addInt(v,1);
	vec_addInt(v,2);
	vec_addInt(v,3);
	sVector *v2 = vec_copy(v);
	u32 e,e2,i = 1;
	vforeach(v,e) {
		u32 j = 1;
		vforeach(v2,e2) {
			test_assertUInt(e2,j);
			j++;
		}
		test_assertUInt(e,i);
		i++;
	}
	vec_destroy(v2,false);
	vec_destroy(v,false);

	test_caseSucceded();
}

static void test_sort(void) {
	test_caseStart("Testing sort");

	sVector *v = vec_create(sizeof(u32));
	vec_addInt(v,3);
	vec_addInt(v,4);
	vec_addInt(v,1);
	vec_addInt(v,6);
	vec_addInt(v,5);
	vec_addInt(v,2);
	vec_addInt(v,7);
	vec_sort(v);
	u32 e,i = 1;
	vforeach(v,e) {
		test_assertUInt(e,i);
		i++;
	}
	vec_destroy(v,false);

	test_caseSucceded();
}

static void test_remove(void) {
	test_caseStart("Testing remove");

	sVector *v = vec_create(sizeof(u32));
	vec_addInt(v,1);
	vec_addInt(v,2);
	vec_addInt(v,3);
	vec_addInt(v,4);
	vec_remove(v,0,1);
	test_assertUInt(v->count,3);
	vec_removeInt(v,3);
	test_assertUInt(v->count,2);
	vec_removeInt(v,4);
	test_assertUInt(v->count,1);
	vec_removeInt(v,2);
	test_assertUInt(v->count,0);
	vec_destroy(v,false);

	test_caseSucceded();
}
