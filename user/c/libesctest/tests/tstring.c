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
#include <esc/util/string.h>
#include <esc/test.h>
#include <stdlib.h>
#include "tstring.h"

/* forward declarations */
static void test_string(void);
static void test_create(void);
static void test_insert(void);
static void test_delete(void);

/* our test-module */
sTestModule tModString = {
	"String",
	&test_string
};

static void test_string(void) {
	test_create();
	test_insert();
	test_delete();
}

static void test_create(void) {
	test_caseStart("Testing create & destroy");
	u32 before = heapspace();
	sString *s1 = str_create();
	test_assertUInt(s1->len,0);
	str_destroy(s1);
	test_assertUInt(heapspace(),before);

	before = heapspace();
	s1 = str_copy("my string");
	test_assertUInt(s1->len,9);
	test_assertStr(s1->str,"my string");
	str_destroy(s1);
	test_assertUInt(heapspace(),before);

	before = heapspace();
	s1 = str_link((char*)"my link");
	test_assertUInt(s1->len,7);
	test_assertStr(s1->str,"my link");
	test_assertUInt(s1->size,0);
	str_destroy(s1);
	test_assertUInt(heapspace(),before);

	test_caseSucceded();
}

static void test_insert(void) {
	test_caseStart("Testing insert & append");

	sString *s = str_create();
	str_append(s,"test");
	test_assertUInt(s->len,4);
	test_assertStr(s->str,"test");
	str_append(s,"noch einer");
	test_assertUInt(s->len,14);
	test_assertStr(s->str,"testnoch einer");
	str_insert(s,0,"abc");
	test_assertUInt(s->len,17);
	test_assertStr(s->str,"abctestnoch einer");
	str_insert(s,1,"def");
	test_assertUInt(s->len,20);
	test_assertStr(s->str,"adefbctestnoch einer");
	str_insert(s,19,"ghi");
	test_assertUInt(s->len,23);
	test_assertStr(s->str,"adefbctestnoch eineghir");
	str_insert(s,23,"jkl");
	test_assertUInt(s->len,26);
	test_assertStr(s->str,"adefbctestnoch eineghirjkl");
	str_destroy(s);

	test_caseSucceded();
}

static void test_delete(void) {
	test_caseStart("Testing delete");

	sString *s = str_copy("mein test string");
	str_delete(s,0,1);
	test_assertUInt(s->len,15);
	test_assertStr(s->str,"ein test string");
	str_delete(s,0,1);
	test_assertUInt(s->len,14);
	test_assertStr(s->str,"in test string");
	str_delete(s,2,2);
	test_assertUInt(s->len,12);
	test_assertStr(s->str,"inest string");
	str_delete(s,9,3);
	test_assertUInt(s->len,9);
	test_assertStr(s->str,"inest str");
	str_delete(s,0,9);
	test_assertUInt(s->len,0);
	test_assertStr(s->str,"");
	str_destroy(s);

	test_caseSucceded();
}
