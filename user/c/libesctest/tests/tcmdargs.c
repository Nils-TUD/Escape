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
#include <esc/util/cmdargs.h>
#include <esc/exceptions/cmdargs.h>
#include <test.h>
#include <errors.h>
#include <stdlib.h>
#include "tcmdargs.h"

/* forward declarations */
static void test_cmdargs(void);
static void test_flags(void);
static void test_vals(void);
static void test_reqNFree(void);

/* our test-module */
sTestModule tModCmdArgs = {
	"Command arguments",
	&test_cmdargs
};

static void test_cmdargs(void) {
	test_flags();
	test_vals();
	test_reqNFree();
}

static void test_flags(void) {
	u32 before;
	test_caseStart("Testing flags");

	before = heapspace();
	{
		const char *argv1[] = {"progname",NULL};
		const char *argv2[] = {"progname","-flag",NULL};
		const char *argv3[] = {"progname","--flag1x","--flag2",NULL};
		const char *argv4[] = {"progname","--flag1","--flag2",NULL};
		const char *format = "flag1 flag2";
		bool flag1,flag2;
		sCmdArgs *a;

		flag1 = flag2 = false;
		a = cmdargs_create(ARRAY_SIZE(argv1) - 1,argv1,0);
		a->parse(a,format,&flag1,&flag2);
		test_assertFalse(flag1);
		test_assertFalse(flag2);
		a->destroy(a);

		flag1 = flag2 = false;
		a = cmdargs_create(ARRAY_SIZE(argv2) - 1,argv2,0);
		a->parse(a,format,&flag1,&flag2);
		test_assertFalse(flag1);
		test_assertFalse(flag2);
		a->destroy(a);

		flag1 = flag2 = false;
		a = cmdargs_create(ARRAY_SIZE(argv3) - 1,argv3,0);
		a->parse(a,format,&flag1,&flag2);
		test_assertFalse(flag1);
		test_assertTrue(flag2);
		a->destroy(a);

		flag1 = flag2 = false;
		a = cmdargs_create(ARRAY_SIZE(argv4) - 1,argv4,0);
		a->parse(a,format,&flag1,&flag2);
		test_assertTrue(flag1);
		test_assertTrue(flag2);
		a->destroy(a);
	}
	test_assertUInt(heapspace(),before);

	test_caseSucceded();
}

static void test_vals(void) {
	u32 before;
	test_caseStart("Testing values");

	before = heapspace();
	{
		const char *argv1[] = {"progname",NULL};
		const char *argv2[] = {"progname","-a","test","-b=4","-c","-12","-d","0xabc","--long=4",NULL};
		const char *argv3[] = {"progname","-a=test","--long","444",NULL};
		const char *format = "a=s b=d c=i d=x long=d";
		char *a;
		s32 b,c,_long;
		u32 d;
		sCmdArgs *args;

		a = NULL;
		b = c = _long = d = 0;
		args = cmdargs_create(ARRAY_SIZE(argv1) - 1,argv1,0);
		args->parse(args,format,&a,&b,&c,&d,&_long);
		test_assertTrue(a == NULL);
		test_assertInt(b,0);
		test_assertInt(c,0);
		test_assertUInt(d,0);
		test_assertInt(_long,0);
		args->destroy(args);

		a = NULL;
		b = c = _long = d = 0;
		args = cmdargs_create(ARRAY_SIZE(argv2) - 1,argv2,0);
		args->parse(args,format,&a,&b,&c,&d,&_long);
		test_assertStr(a,"test");
		test_assertInt(b,4);
		test_assertInt(c,-12);
		test_assertUInt(d,0xabc);
		test_assertInt(_long,4);
		args->destroy(args);

		a = NULL;
		b = c = _long = d = 0;
		args = cmdargs_create(ARRAY_SIZE(argv3) - 1,argv3,0);
		args->parse(args,format,&a,&b,&c,&d,&_long);
		test_assertStr(a,"test");
		test_assertInt(b,0);
		test_assertInt(c,0);
		test_assertUInt(d,0);
		test_assertInt(_long,444);
		args->destroy(args);
	}
	test_assertUInt(heapspace(),before);

	test_caseSucceded();
}

static void test_reqNFree(void) {
	u32 before;
	test_caseStart("Testing required and free args");

	before = heapspace();
	{
		const char *argv1[] = {"progname",NULL};
		const char *argv2[] = {"progname","-b","12",NULL};
		const char *argv3[] = {"progname","-b=123","--req","val",NULL};
		const char *argv4[] = {"progname","-b=12","--nreq=f","--req=test","free1","-free2","--req",NULL};
		const char *format = "b=d* req=s* nreq=X";
		sIterator it;
		bool ex;
		s32 b;
		char *req;
		u32 nreq;
		sCmdArgs *a;

		TRY {
			b = nreq = ex = 0;
			req = NULL;
			a = cmdargs_create(ARRAY_SIZE(argv1) - 1,argv1,0);
			a->parse(a,format,&b,&req,&nreq);
		}
		CATCH(CmdArgsException,e) {
			ex = true;
		}
		ENDCATCH
		it = a->getFreeArgs(a);
		test_assertFalse(it.hasNext(&it));
		test_assertTrue(ex);
		test_assertTrue(req == NULL);
		test_assertInt(b,0);
		test_assertUInt(nreq,0);
		a->destroy(a);

		/* --- */

		TRY {
			b = nreq = ex = 0;
			req = NULL;
			a = cmdargs_create(ARRAY_SIZE(argv2) - 1,argv2,0);
			a->parse(a,format,&b,&req,&nreq);
		}
		CATCH(CmdArgsException,e) {
			ex = true;
		}
		ENDCATCH
		it = a->getFreeArgs(a);
		test_assertFalse(it.hasNext(&it));
		test_assertTrue(ex);
		test_assertTrue(req == NULL);
		test_assertInt(b,12);
		test_assertUInt(nreq,0);
		a->destroy(a);

		/* --- */

		TRY {
			b = nreq = ex = 0;
			req = NULL;
			a = cmdargs_create(ARRAY_SIZE(argv3) - 1,argv3,0);
			a->parse(a,format,&b,&req,&nreq);
		}
		CATCH(CmdArgsException,e) {
			ex = true;
		}
		ENDCATCH
		it = a->getFreeArgs(a);
		test_assertFalse(it.hasNext(&it));
		test_assertFalse(ex);
		test_assertStr(req,"val");
		test_assertInt(b,123);
		test_assertUInt(nreq,0);
		a->destroy(a);

		/* --- */

		TRY {
			b = nreq = ex = 0;
			req = NULL;
			a = cmdargs_create(ARRAY_SIZE(argv4) - 1,argv4,0);
			a->parse(a,format,&b,&req,&nreq);
		}
		CATCH(CmdArgsException,e) {
			ex = true;
		}
		ENDCATCH
		it = a->getFreeArgs(a);
		test_assertStr(it.next(&it),"free1");
		test_assertStr(it.next(&it),"-free2");
		test_assertStr(it.next(&it),"--req");
		test_assertFalse(it.hasNext(&it));
		test_assertFalse(ex);
		test_assertStr(req,"test");
		test_assertInt(b,12);
		test_assertUInt(nreq,0xf);
		a->destroy(a);
	}
	test_assertUInt(heapspace(),before);

	test_caseSucceded();
}
