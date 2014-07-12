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
#include <usergroup/passwd.h>
#include <sys/test.h>
#include <stdlib.h>

static void test_passwd(void);
static void test_basics(void);
static void test_errors(void);

/* our test-module */
sTestModule tModPasswd = {
	"Passwd",
	&test_passwd
};

static void test_passwd(void) {
	test_basics();
	test_errors();
}

static void test_basics(void) {
	size_t count,oldFree;
	test_caseStart("Testing basics");

	{
		sPasswd *p;
		oldFree = heapspace();
		p = pw_parse("0:root",&count);
		test_assertTrue(p != NULL);
		test_assertSize(count,1);
		if(p) {
			test_assertTrue(p->next == NULL);
			test_assertUInt(p->uid,0);
			test_assertStr(p->pw,"root");
		}
		pw_free(p);
		test_assertSize(heapspace(),oldFree);
	}

	{
		sPasswd *p;
		oldFree = heapspace();
		p = pw_parse("122:a",&count);
		test_assertTrue(p != NULL);
		test_assertSize(count,1);
		if(p) {
			test_assertTrue(p->next == NULL);
			test_assertUInt(p->uid,122);
			test_assertStr(p->pw,"a");
		}
		pw_free(p);
		test_assertSize(heapspace(),oldFree);
	}

	{
		sPasswd *p,*res;
		oldFree = heapspace();
		res = pw_parse("122:a\n\n4:test",&count);
		p = res;
		test_assertTrue(p != NULL);
		test_assertSize(count,2);
		if(p) {
			test_assertTrue(p->next != NULL);
			test_assertUInt(p->uid,122);
			test_assertStr(p->pw,"a");
			p = p->next;
		}
		test_assertTrue(p != NULL);
		if(p) {
			test_assertTrue(p->next == NULL);
			test_assertUInt(p->uid,4);
			test_assertStr(p->pw,"test");
		}
		pw_free(res);
		test_assertSize(heapspace(),oldFree);
	}

	test_caseSucceeded();
}

static void test_errors(void) {
	const char *errors[] = {
		"",
		"::",
		":root",
		"0::",
	};
	size_t i,count,oldFree;
	test_caseStart("Testing errors");

	for(i = 0; i < ARRAY_SIZE(errors); i++) {
		sPasswd *p;
		oldFree = heapspace();
		p = pw_parse(errors[i],&count);
		test_assertTrue(p == NULL);
		test_assertSize(heapspace(),oldFree);
	}

	test_caseSucceeded();
}
