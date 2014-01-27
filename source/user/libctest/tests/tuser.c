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

#include <esc/common.h>
#include <usergroup/user.h>
#include <esc/test.h>
#include <stdlib.h>

static void test_user(void);
static void test_basics(void);
static void test_errors(void);

/* our test-module */
sTestModule tModUser = {
	"User",
	&test_user
};

static void test_user(void) {
	test_basics();
	test_errors();
}

static void test_basics(void) {
	size_t count,oldFree;
	test_caseStart("Testing basics");

	{
		sUser *u;
		oldFree = heapspace();
		u = user_parse("0:0:root:/root",&count);
		test_assertTrue(u != NULL);
		test_assertSize(count,1);
		if(u) {
			test_assertTrue(u->next == NULL);
			test_assertUInt(u->uid,0);
			test_assertUInt(u->gid,0);
			test_assertStr(u->name,"root");
			test_assertStr(u->home,"/root");
		}
		user_free(u);
		test_assertSize(heapspace(),oldFree);
	}

	{
		sUser *u;
		oldFree = heapspace();
		u = user_parse("122:444:a:c",&count);
		test_assertTrue(u != NULL);
		test_assertSize(count,1);
		if(u) {
			test_assertTrue(u->next == NULL);
			test_assertUInt(u->uid,122);
			test_assertUInt(u->gid,444);
			test_assertStr(u->name,"a");
			test_assertStr(u->home,"c");
		}
		user_free(u);
		test_assertSize(heapspace(),oldFree);
	}

	{
		sUser *u,*res;
		oldFree = heapspace();
		res = user_parse("122:4:a:c\n\n4:10:test:/test",&count);
		u = res;
		test_assertTrue(u != NULL);
		test_assertSize(count,2);
		if(u) {
			test_assertTrue(u->next != NULL);
			test_assertUInt(u->uid,122);
			test_assertUInt(u->gid,4);
			test_assertStr(u->name,"a");
			test_assertStr(u->home,"c");
			u = u->next;
		}
		test_assertTrue(u != NULL);
		if(u) {
			test_assertTrue(u->next == NULL);
			test_assertUInt(u->uid,4);
			test_assertUInt(u->gid,10);
			test_assertStr(u->name,"test");
			test_assertStr(u->home,"/test");
		}
		user_free(res);
		test_assertSize(heapspace(),oldFree);
	}

	{
		sUser *u,*res;
		oldFree = heapspace();
		res = user_parse("0:0:root:/root\n1:1:hrniels:/home/hrniels\n",&count);
		u = res;
		test_assertTrue(u != NULL);
		test_assertSize(count,2);
		if(u) {
			test_assertTrue(u->next != NULL);
			test_assertUInt(u->uid,0);
			test_assertUInt(u->gid,0);
			test_assertStr(u->name,"root");
			test_assertStr(u->home,"/root");
			u = u->next;
		}
		test_assertTrue(u != NULL);
		if(u) {
			test_assertTrue(u->next == NULL);
			test_assertUInt(u->uid,1);
			test_assertUInt(u->gid,1);
			test_assertStr(u->name,"hrniels");
			test_assertStr(u->home,"/home/hrniels");
		}
		user_free(res);
		test_assertSize(heapspace(),oldFree);
	}

	test_caseSucceeded();
}

static void test_errors(void) {
	const char *errors[] = {
		"",
		"::",
		":root:/root",
		"0:1::",
		"0:1:a::",
		"0:1:::b",
		"0:0::b:",
		"0:10:b:",
	};
	size_t i,count,oldFree;
	test_caseStart("Testing errors");

	for(i = 0; i < ARRAY_SIZE(errors); i++) {
		sUser *u;
		oldFree = heapspace();
		u = user_parse(errors[i],&count);
		test_assertTrue(u == NULL);
		test_assertSize(heapspace(),oldFree);
	}

	test_caseSucceeded();
}
