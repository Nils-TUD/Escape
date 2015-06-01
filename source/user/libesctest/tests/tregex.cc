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

#include <esc/stream/std.h>
#include <esc/regex/regex.h>
#include <sys/common.h>
#include <sys/test.h>
#include <math.h>

using namespace esc;

static void test_basic();
static void test_repeat();
static void test_dot();
static void test_groups();
static void test_charclass();
static void test_errors();
static void test_regex();

/* our test-module */
sTestModule tModRegex = {
    "Regex",
    &test_regex
};

static void test_regex() {
    test_basic();
    test_repeat();
    test_dot();
    test_groups();
    test_charclass();
    test_errors();
}

static void test_basic() {
	test_caseStart("Testing basic functionality");

	size_t before = heapspace();
	{
		test_assertTrue(Regex::matches("",""));
		test_assertFalse(Regex::matches("","a"));
	}

	{
		Regex::Pattern pat = Regex::compile("a");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertFalse(Regex::matches(pat,"b"));
		test_assertFalse(Regex::matches(pat,""));
	}

	{
		Regex::Pattern pat = Regex::compile("\\[a-z\\]");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"[a-z]"));
		test_assertFalse(Regex::matches(pat,"a"));
	}

	{
		Regex::Pattern pat = Regex::compile("a\\*b\\{2\\}");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a*b{2}"));
		test_assertFalse(Regex::matches(pat,"abb"));
	}

	{
		Regex::Pattern pat = Regex::compile("[\\^\\.\\*]");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"^"));
		test_assertTrue(Regex::matches(pat,"."));
		test_assertTrue(Regex::matches(pat,"*"));
	}
	// we extend the heap here, so don't require it to be equal
	test_assertTrue(heapspace() >= before);

	test_caseSucceeded();
}

static void test_repeat() {
	test_caseStart("Testing repeat operators");

	size_t before = heapspace();
	{
		Regex::Pattern pat = Regex::compile("a*");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertTrue(Regex::matches(pat,"aa"));
		test_assertTrue(Regex::matches(pat,"aaaa"));
		test_assertTrue(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"ab"));
	}

	{
		Regex::Pattern pat = Regex::compile("a*b*c?de+");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"abcde"));
		test_assertTrue(Regex::matches(pat,"deee"));
		test_assertTrue(Regex::matches(pat,"aaabbbdeee"));
		test_assertTrue(Regex::matches(pat,"abdeee"));
		test_assertFalse(Regex::matches(pat,"abdef"));
		test_assertFalse(Regex::matches(pat,"a"));
		test_assertFalse(Regex::matches(pat,"ab"));
		test_assertFalse(Regex::matches(pat,"abc"));
		test_assertFalse(Regex::matches(pat,"abe"));
		test_assertFalse(Regex::matches(pat,"abce"));
	}

	{
		Regex::Pattern pat = Regex::compile("a{1,2}");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertTrue(Regex::matches(pat,"aa"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"aaa"));
		test_assertFalse(Regex::matches(pat,"b"));
	}
	test_assertSize(heapspace(),before);

	{
		Regex::Pattern pat = Regex::compile("a{4,123}");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"aaaa"));
		test_assertTrue(Regex::matches(pat,"aaaaaaaaaa"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"aa"));
		test_assertFalse(Regex::matches(pat,"b"));
	}
	test_assertSize(heapspace(),before);

	{
		Regex::Pattern pat = Regex::compile("a{2,}");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"aa"));
		test_assertTrue(Regex::matches(pat,"aaa"));
		test_assertTrue(Regex::matches(pat,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"a"));
		test_assertFalse(Regex::matches(pat,"b"));
	}
	test_assertSize(heapspace(),before);

	test_caseSucceeded();
}

static void test_dot() {
	test_caseStart("Testing dot operator");

	size_t before = heapspace();
	{
		Regex::Pattern pat = Regex::compile(".");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertTrue(Regex::matches(pat,"b"));
		test_assertTrue(Regex::matches(pat,"b"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"aa"));
	}

	{
		Regex::Pattern pat = Regex::compile("a.*");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertTrue(Regex::matches(pat,"aa"));
		test_assertTrue(Regex::matches(pat,"abc"));
		test_assertTrue(Regex::matches(pat,"a123213213"));
		test_assertFalse(Regex::matches(pat,""));
	}
	test_assertSize(heapspace(),before);

	test_caseSucceeded();
}

static void test_groups() {
	test_caseStart("Testing groups");

	size_t before = heapspace();
	{
		Regex::Pattern pat = Regex::compile("(.)");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertTrue(Regex::matches(pat,"b"));
		test_assertTrue(Regex::matches(pat,"b"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"aa"));

		Regex::Result res = Regex::execute(pat,"a");
		test_assertTrue(res.matched());
		test_assertSize(res.groups(),2);
		test_assertStr(res.get(0).c_str(), "a");
		test_assertStr(res.get(1).c_str(), "a");
	}

	{
		Regex::Pattern pat = Regex::compile("((.))");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertTrue(Regex::matches(pat,"b"));
		test_assertTrue(Regex::matches(pat,"b"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"aa"));

		Regex::Result res = Regex::execute(pat,"b");
		test_assertTrue(res.matched());
		test_assertSize(res.groups(),3);
		test_assertStr(res.get(0).c_str(), "b");
		test_assertStr(res.get(1).c_str(), "b");
		test_assertStr(res.get(2).c_str(), "b");
	}

	{
		Regex::Pattern pat = Regex::compile("(ab)+c(d*)");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"abc"));
		test_assertTrue(Regex::matches(pat,"abababc"));
		test_assertTrue(Regex::matches(pat,"ababcddd"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"aa"));
		test_assertFalse(Regex::matches(pat,"aba"));
		test_assertFalse(Regex::matches(pat,"abdd"));

		Regex::Result res = Regex::execute(pat,"ababcddd");
		test_assertTrue(res.matched());
		test_assertSize(res.groups(),3);
		test_assertStr(res.get(0).c_str(), "ababcddd");
		test_assertStr(res.get(1).c_str(), "ab");
		test_assertStr(res.get(2).c_str(), "ddd");
	}

	{
		Regex::Pattern pat = Regex::compile("(1(ab)+(cd)+2)+");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"1abcd2"));
		test_assertTrue(Regex::matches(pat,"1abcd21abcd2"));
		test_assertTrue(Regex::matches(pat,"1ababcdcd21ababcd2"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"1a"));
		test_assertFalse(Regex::matches(pat,"1ab"));
		test_assertFalse(Regex::matches(pat,"1abcd"));
		test_assertFalse(Regex::matches(pat,"12"));
		test_assertFalse(Regex::matches(pat,"abcd"));

		Regex::Result res = Regex::execute(pat,"1abcd21abcd2");
		test_assertTrue(res.matched());
		test_assertSize(res.groups(),4);
		test_assertStr(res.get(0).c_str(), "1abcd21abcd2");
		test_assertStr(res.get(1).c_str(), "1abcd2");
		test_assertStr(res.get(2).c_str(), "ab");
		test_assertStr(res.get(3).c_str(), "cd");
	}

	{
		Regex::Pattern pat = Regex::compile("a([^c]{2})+c");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"abdefc"));
		test_assertTrue(Regex::matches(pat,"abbc"));
		test_assertTrue(Regex::matches(pat,"abdde__c"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"ac"));
		test_assertFalse(Regex::matches(pat,"abc"));

		Regex::Result res = Regex::execute(pat,"abbeeddc");
		test_assertTrue(res.matched());
		test_assertSize(res.groups(),2);
		test_assertStr(res.get(0).c_str(), "abbeeddc");
		test_assertStr(res.get(1).c_str(), "dd");
	}
	test_assertSize(heapspace(),before);

	test_caseSucceeded();
}

static void test_charclass() {
	test_caseStart("Testing character classes");

	size_t before = heapspace();
	{
		Regex::Pattern pat = Regex::compile("[a]");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"b"));
		test_assertFalse(Regex::matches(pat,"ab"));
	}

	{
		Regex::Pattern pat = Regex::compile("[abc]");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertTrue(Regex::matches(pat,"b"));
		test_assertTrue(Regex::matches(pat,"c"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"d"));
		test_assertFalse(Regex::matches(pat,"e"));
	}

	{
		Regex::Pattern pat = Regex::compile("[a-z934]+");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"abtzaasd"));
		test_assertTrue(Regex::matches(pat,"bbas3999444"));
		test_assertTrue(Regex::matches(pat,"a"));
		test_assertFalse(Regex::matches(pat,""));
		test_assertFalse(Regex::matches(pat,"A"));
		test_assertFalse(Regex::matches(pat,"123"));
	}

	{
		Regex::Pattern pat = Regex::compile("[^a-z]+");
		// sout << pat << "\n";
		test_assertTrue(Regex::matches(pat,"0123"));
		test_assertTrue(Regex::matches(pat,"ABC"));
		test_assertTrue(Regex::matches(pat,"/"));
		test_assertFalse(Regex::matches(pat,"a"));
		test_assertFalse(Regex::matches(pat,"abc"));
		test_assertFalse(Regex::matches(pat,"zzz"));
	}
	test_assertSize(heapspace(),before);

	test_caseSucceeded();
}

static void assert_compileFail(const std::string &pattern) {
	try {
		Regex::Pattern pat = Regex::compile(pattern);
		// sout << pat << "\n";
		test_assertFalse(true);
	}
	catch(const std::runtime_error &e) {
		// sout << e.what() << "\n";
		test_assertTrue(true);
	}
}

static void test_errors() {
	test_caseStart("Testing errors");

	size_t before = heapspace();
	assert_compileFail("*");
	assert_compileFail(".*+");
	assert_compileFail(".*+?");
	assert_compileFail(".*+?");
	assert_compileFail(")");
	assert_compileFail("(");
	assert_compileFail("(()");
	assert_compileFail(".*)");
	assert_compileFail("((.*)+)**");
	assert_compileFail("((.*)+**");
	assert_compileFail("]");
	assert_compileFail("[");
	assert_compileFail("[abc");
	assert_compileFail("[abc-");
	assert_compileFail("{1,2}");
	assert_compileFail("a{,}");
	assert_compileFail("a{,3}");
	assert_compileFail("a{2,3");
	assert_compileFail("a{a,4}");
	assert_compileFail("a{4,2}");
	assert_compileFail("a{0,0}");
	assert_compileFail("a}");
	test_assertSize(heapspace(),before);

	test_caseSucceeded();
}
