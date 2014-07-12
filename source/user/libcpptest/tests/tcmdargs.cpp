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
#include <cmdargs.h>
#include <sys/test.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

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
	size_t before;
	test_caseStart("Testing flags");

	before = heapspace();
	{
		const char * const argv1[] = {"progname",nullptr};
		const char * const argv2[] = {"progname","-flag",nullptr};
		const char * const argv3[] = {"progname","--flag1x","--flag2",nullptr};
		const char * const argv4[] = {"progname","--flag1","--flag2",nullptr};
		const char *format = "flag1 flag2";
		int flag1,flag2;
		cmdargs *a;

		flag1 = flag2 = false;
		a = new cmdargs(ARRAY_SIZE(argv1) - 1,(char* const*)argv1,0);
		a->parse(format,&flag1,&flag2);
		test_assertFalse(flag1);
		test_assertFalse(flag2);
		delete a;

		flag1 = flag2 = false;
		a = new cmdargs(ARRAY_SIZE(argv2) - 1,(char* const*)argv2,0);
		a->parse(format,&flag1,&flag2);
		test_assertFalse(flag1);
		test_assertFalse(flag2);
		delete a;

		flag1 = flag2 = false;
		a = new cmdargs(ARRAY_SIZE(argv3) - 1,(char* const*)argv3,0);
		a->parse(format,&flag1,&flag2);
		test_assertFalse(flag1);
		test_assertTrue(flag2);
		delete a;

		flag1 = flag2 = false;
		a = new cmdargs(ARRAY_SIZE(argv4) - 1,(char* const*)argv4,0);
		a->parse(format,&flag1,&flag2);
		test_assertTrue(flag1);
		test_assertTrue(flag2);
		delete a;
	}
	test_assertSize(heapspace(),before);

	test_caseSucceeded();
}

static void test_vals(void) {
	size_t before;
	test_caseStart("Testing values");

	before = heapspace();
	{
		const char * const argv1[] = {"progname",nullptr};
		const char * const argv2[] = {"progname","-a","test","-b=4","-c","-12","-d","0xabc","--long=4",nullptr};
		const char * const argv3[] = {"progname","-a=test","--long","444",nullptr};
		const char *format = "a=s b=d c=i d=x long=d";
		string a;
		int b,c,_long;
		uint d;
		cmdargs *args;

		a = "";
		b = c = _long = d = 0;
		args = new cmdargs(ARRAY_SIZE(argv1) - 1,(char* const*)argv1,0);
		args->parse(format,&a,&b,&c,&d,&_long);
		test_assertTrue(a == "");
		test_assertInt(b,0);
		test_assertInt(c,0);
		test_assertUInt(d,0);
		test_assertInt(_long,0);
		delete args;

		a = "";
		b = c = _long = d = 0;
		args = new cmdargs(ARRAY_SIZE(argv2) - 1,(char* const*)argv2,0);
		args->parse(format,&a,&b,&c,&d,&_long);
		test_assertStr(a.c_str(),"test");
		test_assertInt(b,4);
		test_assertInt(c,-12);
		test_assertUInt(d,0xabc);
		test_assertInt(_long,4);
		delete args;

		a = "";
		b = c = _long = d = 0;
		args = new cmdargs(ARRAY_SIZE(argv3) - 1,(char* const*)argv3,0);
		args->parse(format,&a,&b,&c,&d,&_long);
		test_assertStr(a.c_str(),"test");
		test_assertInt(b,0);
		test_assertInt(c,0);
		test_assertUInt(d,0);
		test_assertInt(_long,444);
		delete args;
	}
	test_assertUInt(heapspace(),before);

	test_caseSucceeded();
}

static void test_reqNFree(void) {
	size_t before;
	test_caseStart("Testing required and free args");

	// one exception before to allocate mem for exception-stuff
	// this way we can test whether everything that we've under control is free'd
	try {
		throw cmdargs_error("");
	}
	catch(...) {
	}

	before = heapspace();
	{
		const char * const argv1[] = {"progname",nullptr};
		const char * const argv2[] = {"progname","-b","12",nullptr};
		const char * const argv3[] = {"progname","-b=123","--req","val",nullptr};
		const char * const argv4[] = {"progname","-b=12","--nreq=f","--req=test","free1","-free2","--req",nullptr};
		const char *format = "b=d* req=s* nreq=X";
		int ex;
		int b;
		string req;
		uint nreq;
		cmdargs *a = nullptr;

		try {
			b = nreq = ex = 0;
			req = "";
			a = new cmdargs(ARRAY_SIZE(argv1) - 1,(char* const*)argv1,0);
			a->parse(format,&b,&req,&nreq);
		}
		catch(const cmdargs_error& e) {
			ex = true;
		}
		test_assertSize(a->get_free().size(),0);
		test_assertTrue(ex);
		test_assertTrue(req == "");
		test_assertInt(b,0);
		test_assertUInt(nreq,0);
		delete a;

		/* --- */

		try {
			b = nreq = ex = 0;
			req = "";
			a = new cmdargs(ARRAY_SIZE(argv2) - 1,(char* const*)argv2,0);
			a->parse(format,&b,&req,&nreq);
		}
		catch(const cmdargs_error& e) {
			ex = true;
		}
		test_assertSize(a->get_free().size(),0);
		test_assertTrue(ex);
		test_assertTrue(req == "");
		test_assertInt(b,12);
		test_assertUInt(nreq,0);
		delete a;

		/* --- */

		try {
			b = nreq = ex = 0;
			req = "";
			a = new cmdargs(ARRAY_SIZE(argv3) - 1,(char* const*)argv3,0);
			a->parse(format,&b,&req,&nreq);
		}
		catch(const cmdargs_error& e) {
			ex = true;
		}
		test_assertSize(a->get_free().size(),0);
		test_assertFalse(ex);
		test_assertStr(req.c_str(),"val");
		test_assertInt(b,123);
		test_assertUInt(nreq,0);
		delete a;

		/* --- */

		try {
			b = nreq = ex = 0;
			req = "";
			a = new cmdargs(ARRAY_SIZE(argv4) - 1,(char* const*)argv4,0);
			a->parse(format,&b,&req,&nreq);
		}
		catch(const cmdargs_error& e) {
			ex = true;
		}
		const vector<string*>& fargs = a->get_free();
		test_assertSize(fargs.size(),3);
		test_assertStr(fargs[0]->c_str(),"free1");
		test_assertStr(fargs[1]->c_str(),"-free2");
		test_assertStr(fargs[2]->c_str(),"--req");
		test_assertFalse(ex);
		test_assertStr(req.c_str(),"test");
		test_assertInt(b,12);
		test_assertUInt(nreq,0xf);
		delete a;
	}
	test_assertSize(heapspace(),before);

	test_caseSucceeded();
}
