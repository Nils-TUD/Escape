/**
 * $Id$
 */

#include <esc/common.h>
#include <usergroup/user.h>
#include <esc/test.h>
#include <stdlib.h>
#include "tuser.h"

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
		oldFree = heapspace();
		sUser *u = user_parse("0:0:root:root:/root",&count);
		test_assertTrue(u != NULL);
		test_assertSize(count,1);
		if(u) {
			test_assertTrue(u->next == NULL);
			test_assertUInt(u->uid,0);
			test_assertUInt(u->gid,0);
			test_assertStr(u->name,"root");
			test_assertStr(u->pw,"root");
			test_assertStr(u->home,"/root");
		}
		user_free(u);
		test_assertSize(heapspace(),oldFree);
	}

	{
		oldFree = heapspace();
		sUser *u = user_parse("122:444:a:b:c",&count);
		test_assertTrue(u != NULL);
		test_assertSize(count,1);
		if(u) {
			test_assertTrue(u->next == NULL);
			test_assertUInt(u->uid,122);
			test_assertUInt(u->gid,444);
			test_assertStr(u->name,"a");
			test_assertStr(u->pw,"b");
			test_assertStr(u->home,"c");
		}
		user_free(u);
		test_assertSize(heapspace(),oldFree);
	}

	{
		oldFree = heapspace();
		sUser *res = user_parse("122:4:a:b:c\n\n4:10:test:1:/test",&count);
		sUser *u = res;
		test_assertTrue(u != NULL);
		test_assertSize(count,2);
		if(u) {
			test_assertTrue(u->next != NULL);
			test_assertUInt(u->uid,122);
			test_assertUInt(u->gid,4);
			test_assertStr(u->name,"a");
			test_assertStr(u->pw,"b");
			test_assertStr(u->home,"c");
			u = u->next;
		}
		test_assertTrue(u != NULL);
		if(u) {
			test_assertTrue(u->next == NULL);
			test_assertUInt(u->uid,4);
			test_assertUInt(u->gid,10);
			test_assertStr(u->name,"test");
			test_assertStr(u->pw,"1");
			test_assertStr(u->home,"/test");
		}
		user_free(res);
		test_assertSize(heapspace(),oldFree);
	}

	{
		oldFree = heapspace();
		sUser *res = user_parse("0:0:root:root:/root\n1:1:hrniels:test:/home/hrniels\n",&count);
		sUser *u = res;
		test_assertTrue(u != NULL);
		test_assertSize(count,2);
		if(u) {
			test_assertTrue(u->next != NULL);
			test_assertUInt(u->uid,0);
			test_assertUInt(u->gid,0);
			test_assertStr(u->name,"root");
			test_assertStr(u->pw,"root");
			test_assertStr(u->home,"/root");
			u = u->next;
		}
		test_assertTrue(u != NULL);
		if(u) {
			test_assertTrue(u->next == NULL);
			test_assertUInt(u->uid,1);
			test_assertUInt(u->gid,1);
			test_assertStr(u->name,"hrniels");
			test_assertStr(u->pw,"test");
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
		":root:root:/root",
		"0:1:::",
		"0:1:a::",
		"0:1:::b",
		"0:0::b:",
		"0:10:a:b:",
		"0:0:a::b",
		"0:0::a:b",
	};
	size_t i,count,oldFree;
	test_caseStart("Testing errors");

	for(i = 0; i < ARRAY_SIZE(errors); i++) {
		oldFree = heapspace();
		sUser *u = user_parse(errors[i],&count);
		test_assertTrue(u == NULL);
		test_assertSize(heapspace(),oldFree);
	}

	test_caseSucceeded();
}
