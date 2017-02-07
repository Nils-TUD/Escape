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
#include <sys/test.h>
#include <getopt.h>

/* forward declarations */
static void test_getopt(void);
static void test_woargs(void);
static void test_wargs(void);
static void test_longopts(void);

/* our test-module */
sTestModule tModGetOpt = {
	"GetOpt",
	&test_getopt
};

static void test_getopt(void) {
	test_woargs();
	test_wargs();
	test_longopts();
}

static void test_woargs(void) {
	test_caseStart("Testing options without arguments");

	int res;

	{
		const char *optstring = "abc";
		const char *args[] = {NULL,"-a","-b","-ccc","-c","-bc","-de","--"};

		optind = 1;
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,2);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'a');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,3);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'b');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,3);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'c');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,3);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'c');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,4);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'c');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,5);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'c');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,5);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'b');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,6);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'c');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,6);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'?');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,7);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'?');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,7);
		test_assertStr(optarg,NULL);
		test_assertInt(res,-1);
	}

	test_caseSucceeded();
}

static void test_wargs(void) {
	test_caseStart("Testing options with arguments");

	int res;

	{
		const char *optstring = "a:b:c";
		const char *args[] = {NULL,"-a","foo","-b","bar","-c","-bzoo","-"};

		optind = 1;
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,3);
		test_assertStr(optarg,"foo");
		test_assertInt(res,'a');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,5);
		test_assertStr(optarg,"bar");
		test_assertInt(res,'b');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,6);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'c');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,7);
		test_assertStr(optarg,"zoo");
		test_assertInt(res,'b');
		res = getopt(ARRAY_SIZE(args),(char**)args,optstring);
		test_assertInt(optind,7);
		test_assertStr(optarg,NULL);
		test_assertInt(res,-1);
	}

	test_caseSucceeded();
}

static void test_longopts(void) {
	test_caseStart("Testing long options");

	int res;

	{
		const char *optstring = "a";
		const struct option longopts[] = {
			{"foo",	no_argument, NULL, 'f'},
			{"bar", required_argument, NULL, 'b'},
			{NULL, 0, NULL, 0},
		};

		const char *args[] = {NULL,"-a","--foo","--bar","test","--bar=zoo"};

		optind = 1;
		res = getopt_long(ARRAY_SIZE(args),(char**)args,optstring,longopts,NULL);
		test_assertInt(optind,2);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'a');
		res = getopt_long(ARRAY_SIZE(args),(char**)args,optstring,longopts,NULL);
		test_assertInt(optind,3);
		test_assertStr(optarg,NULL);
		test_assertInt(res,'f');
		res = getopt_long(ARRAY_SIZE(args),(char**)args,optstring,longopts,NULL);
		test_assertInt(optind,5);
		test_assertStr(optarg,"test");
		test_assertInt(res,'b');
		res = getopt_long(ARRAY_SIZE(args),(char**)args,optstring,longopts,NULL);
		test_assertInt(optind,6);
		test_assertStr(optarg,"zoo");
		test_assertInt(res,'b');
		res = getopt_long(ARRAY_SIZE(args),(char**)args,optstring,longopts,NULL);
		test_assertInt(optind,6);
		test_assertStr(optarg,NULL);
		test_assertInt(res,-1);
	}

	test_caseSucceeded();
}
