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
#include <usergroup/group.h>
#include <esc/test.h>
#include <stdlib.h>
#include "tgroup.h"

static void test_group(void);
static void test_basics(void);
static void test_errors(void);

/* our test-module */
sTestModule tModGroup = {
	"Group",
	&test_group
};

static void test_group(void) {
	test_basics();
	test_errors();
}

static void test_basics(void) {
	size_t count,oldFree;
	test_caseStart("Testing basics");

	{
		oldFree = heapspace();
		sGroup *g = group_parse("0:root:0",&count);
		test_assertTrue(g != NULL);
		test_assertSize(count,1);
		if(g) {
			test_assertTrue(g->next == NULL);
			test_assertUInt(g->gid,0);
			test_assertStr(g->name,"root");
			test_assertSize(g->userCount,1);
			test_assertUInt(g->users[0],0);
		}
		group_free(g);
		test_assertSize(heapspace(),oldFree);
	}

	{
		oldFree = heapspace();
		sGroup *g = group_parse("0:root\n",&count);
		test_assertTrue(g != NULL);
		test_assertSize(count,1);
		if(g) {
			test_assertTrue(g->next == NULL);
			test_assertUInt(g->gid,0);
			test_assertStr(g->name,"root");
			test_assertSize(g->userCount,0);
		}
		group_free(g);
		test_assertSize(heapspace(),oldFree);
	}

	{
		oldFree = heapspace();
		sGroup *g = group_parse("0:root:",&count);
		test_assertTrue(g != NULL);
		test_assertSize(count,1);
		if(g) {
			test_assertTrue(g->next == NULL);
			test_assertUInt(g->gid,0);
			test_assertStr(g->name,"root");
			test_assertSize(g->userCount,0);
		}
		group_free(g);
		test_assertSize(heapspace(),oldFree);
	}

	{
		oldFree = heapspace();
		sGroup *g = group_parse("2444:a:100:200",&count);
		test_assertTrue(g != NULL);
		test_assertSize(count,1);
		if(g) {
			test_assertTrue(g->next == NULL);
			test_assertUInt(g->gid,2444);
			test_assertStr(g->name,"a");
			test_assertSize(g->userCount,2);
			test_assertUInt(g->users[0],100);
			test_assertUInt(g->users[1],200);
		}
		group_free(g);
		test_assertSize(heapspace(),oldFree);
	}

	{
		oldFree = heapspace();
		sGroup *res = group_parse("1:a:1:2\n\n2:b:4",&count);
		test_assertSize(count,2);
		sGroup *g = res;
		test_assertTrue(g != NULL);
		if(g) {
			test_assertUInt(g->gid,1);
			test_assertStr(g->name,"a");
			test_assertSize(g->userCount,2);
			test_assertUInt(g->users[0],1);
			test_assertUInt(g->users[1],2);
			g = g->next;
		}
		test_assertTrue(g != NULL);
		if(g) {
			test_assertUInt(g->gid,2);
			test_assertStr(g->name,"b");
			test_assertSize(g->userCount,1);
			test_assertUInt(g->users[0],4);
		}
		group_free(res);
		test_assertSize(heapspace(),oldFree);
	}

	test_caseSucceeded();
}

static void test_errors(void) {
	const char *errors[] = {
		"",
		":root:0",
		":::",
		"0:::"
	};
	size_t i,count,oldFree;
	test_caseStart("Testing errors");

	for(i = 0; i < ARRAY_SIZE(errors); i++) {
		oldFree = heapspace();
		sGroup *g = group_parse(errors[i],&count);
		test_assertTrue(g == NULL);
		test_assertSize(heapspace(),oldFree);
	}

	test_caseSucceeded();
}
