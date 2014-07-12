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
#include <string>

using namespace std;

/* forward declarations */
static void test_string(void);
static void test_constr(void);
static void test_iterators(void);
static void test_assign(void);
static void test_resize(void);
static void test_clear(void);
static void test_append(void);
static void test_insert(void);
static void test_erase(void);
static void test_replace(void);
static void test_find(void);
static void test_rfind(void);
static void test_find_first_of(void);
static void test_find_last_of(void);
static void test_find_first_not_of(void);
static void test_find_last_not_of(void);
static void test_trim(void);

/* our test-module */
sTestModule tModString = {
	"String",
	&test_string
};

static void test_string(void) {
	test_constr();
	test_iterators();
	test_assign();
	test_resize();
	test_clear();
	test_append();
	test_insert();
	test_erase();
	test_replace();
	test_find();
	test_rfind();
	test_find_first_of();
	test_find_last_of();
	test_find_first_not_of();
	test_find_last_not_of();
	test_trim();
}

static void test_constr(void) {
	test_caseStart("Testing constructors");

	string s1;
	test_assertStr(s1.c_str(),"");
	test_assertSize(s1.length(),0);

	string s1_("mystring");
	test_assertStr(s1_.c_str(),"mystring");
	test_assertSize(s1_.length(),8);

	string s2(s1);
	test_assertStr(s2.c_str(),"");
	test_assertSize(s2.length(),0);

	s2.append("test");
	string s3(s2);
	test_assertStr(s2.c_str(),"test");
	test_assertSize(s2.length(),4);

	string s4(s3,0,4);
	test_assertStr(s4.c_str(),"test");
	test_assertSize(s4.length(),4);
	string s5(s3,1,3);
	test_assertStr(s5.c_str(),"est");
	test_assertSize(s5.length(),3);
	string s6(s3,2,1);
	test_assertStr(s6.c_str(),"s");
	test_assertSize(s6.length(),1);
	string s7(s3,3,0);
	test_assertStr(s7.c_str(),"");
	test_assertSize(s7.length(),0);

	string s8("foo",1);
	test_assertStr(s8.c_str(),"f");
	test_assertSize(s8.length(),1);
	string s9("bar",3);
	test_assertStr(s9.c_str(),"bar");
	test_assertSize(s9.length(),3);
	string s10("bla",0);
	test_assertStr(s10.c_str(),"");
	test_assertSize(s10.length(),0);

	string s11(0,'c');
	test_assertStr(s11.c_str(),"");
	test_assertSize(s11.length(),0);
	string s12(1,'c');
	test_assertStr(s12.c_str(),"c");
	test_assertSize(s12.length(),1);
	string s13(12,'a');
	test_assertStr(s13.c_str(),"aaaaaaaaaaaa");
	test_assertSize(s13.length(),12);

	test_caseSucceeded();
}

static void test_iterators(void) {
	test_caseStart("Testing iterators");

	string s1("0123456789");
	int i = '0';
	for(auto it = s1.begin(); it != s1.end(); ++it)
		test_assertInt(*it,i++);
	i = '9';
	for(auto it = s1.rbegin(); it != s1.rend(); ++it)
		test_assertInt(*it,i--);

	test_caseSucceeded();
}

static void test_assign(void) {
	test_caseStart("Testing assign-operator");

	string s1("test");
	string s1c = s1;
	test_assertStr(s1c.c_str(),"test");
	test_assertSize(s1c.length(),4);

	string s2("foo");
	s2 = s1;
	test_assertStr(s2.c_str(),"test");
	test_assertSize(s2.length(),4);

	s2 = "mystr";
	test_assertStr(s2.c_str(),"mystr");
	test_assertSize(s2.length(),5);

	s2 = 'a';
	test_assertStr(s2.c_str(),"a");
	test_assertSize(s2.length(),1);

	test_caseSucceeded();
}

static void test_resize(void) {
	test_caseStart("Testing resize() and reserve()");

	string s1("abc");
	s1.resize(3);
	test_assertStr(s1.c_str(),"abc");
	test_assertSize(s1.length(),3);

	s1.resize(2);
	test_assertStr(s1.c_str(),"ab");
	test_assertSize(s1.length(),2);

	s1.resize(0);
	test_assertStr(s1.c_str(),"");
	test_assertSize(s1.length(),0);

	s1.resize(12);
	test_assertStr(s1.c_str(),"");
	test_assertSize(s1.length(),12);

	s1.reserve(3);
	test_assertTrue(s1.capacity() >= 3);

	s1.reserve(16);
	test_assertTrue(s1.capacity() >= 16);

	s1.reserve(12);
	test_assertTrue(s1.capacity() >= 12);

	test_caseSucceeded();
}

static void test_clear(void) {
	test_caseStart("Testing clear()");

	string s1("abc");
	s1.clear();
	test_assertStr(s1.c_str(),"");
	test_assertSize(s1.length(),0);

	test_caseSucceeded();
}

static void test_append(void) {
	test_caseStart("Testing append()");

	string s1;
	s1.append("test");
	test_assertStr(s1.c_str(),"test");
	test_assertSize(s1.length(),4);
	s1.append("foo");
	test_assertStr(s1.c_str(),"testfoo");
	test_assertSize(s1.length(),7);
	s1.append("bar");
	test_assertStr(s1.c_str(),"testfoobar");
	test_assertSize(s1.length(),10);

	string s2("abc");
	s2.append(s1);
	test_assertStr(s2.c_str(),"abctestfoobar");
	test_assertSize(s2.length(),13);
	s2.append(s1,0,3);
	test_assertStr(s2.c_str(),"abctestfoobartes");
	test_assertSize(s2.length(),16);
	s2.append(s1,3,5);
	test_assertStr(s2.c_str(),"abctestfoobartestfoob");
	test_assertSize(s2.length(),21);
	s2.append(s1,s1.length() - 1,1);
	test_assertStr(s2.c_str(),"abctestfoobartestfoobr");
	test_assertSize(s2.length(),22);

	string s3;
	s3.append(10,'c');
	test_assertStr(s3.c_str(),"cccccccccc");
	test_assertSize(s3.length(),10);

	string s4;
	s3.append(0,'a');
	test_assertStr(s4.c_str(),"");
	test_assertSize(s4.length(),0);

	test_caseSucceeded();
}

static void test_insert(void) {
	test_caseStart("Testing insert()");

	string s1("test");
	s1.insert(1,"abc");
	test_assertStr(s1.c_str(),"tabcest");
	test_assertSize(s1.length(),7);

	s1.insert(0,"a");
	test_assertStr(s1.c_str(),"atabcest");
	test_assertSize(s1.length(),8);

	s1.insert(4,string("a"));
	test_assertStr(s1.c_str(),"atabacest");
	test_assertSize(s1.length(),9);

	s1.insert(s1.length(),string("foo"),1,1);
	test_assertStr(s1.c_str(),"atabacesto");
	test_assertSize(s1.length(),10);

	test_caseSucceeded();
}

static void test_erase(void) {
	test_caseStart("Testing erase()");

	string s1("test");
	s1.erase();
	test_assertStr(s1.c_str(),"");
	test_assertSize(s1.length(),0);

	string s2("test");
	s2.erase(0,1);
	test_assertStr(s2.c_str(),"est");
	test_assertSize(s2.length(),3);
	s2.erase(1,1);
	test_assertStr(s2.c_str(),"et");
	test_assertSize(s2.length(),2);
	s2.erase(0,2);
	test_assertStr(s2.c_str(),"");
	test_assertSize(s2.length(),0);

	string s3("foobar");
	s3.erase(3,3);
	test_assertStr(s3.c_str(),"foo");
	test_assertSize(s3.length(),3);
	s3.erase(2,2);
	test_assertStr(s3.c_str(),"fo");
	test_assertSize(s3.length(),2);

	string::iterator it;
	string s4("barfoo");
	it = s4.erase(s4.begin() + 3,s4.begin() + 5);
	test_assertInt(*it,'o');
	test_assertStr(s4.c_str(),"baro");
	test_assertSize(s4.length(),4);

	it = s4.erase(s4.end() - 1);
	test_assertUIntPtr((uintptr_t)it,(uintptr_t)s4.end());
	test_assertStr(s4.c_str(),"bar");
	test_assertSize(s4.length(),3);

	it = s4.erase(s4.begin());
	test_assertInt(*it,'a');
	test_assertStr(s4.c_str(),"ar");
	test_assertSize(s4.length(),2);

	test_caseSucceeded();
}

static void test_replace(void) {
	test_caseStart("Testing replace()");

	string s1("test");
	s1.replace(0,2,"foo");
	test_assertStr(s1.c_str(),"foost");
	test_assertSize(s1.length(),5);

	s1.replace((size_t)0,0,"bar");
	test_assertStr(s1.c_str(),"barfoost");
	test_assertSize(s1.length(),8);

	s1.replace(7,1,"test");
	test_assertStr(s1.c_str(),"barfoostest");
	test_assertSize(s1.length(),11);

	s1.replace(11,12,"a");
	test_assertStr(s1.c_str(),"barfoostesta");
	test_assertSize(s1.length(),12);

	s1.replace(s1.begin(),s1.end(),string("foobar"));
	test_assertStr(s1.c_str(),"foobar");
	test_assertSize(s1.length(),6);

	s1.replace(s1.begin() + 1,s1.end() - 1,"foo",2);
	test_assertStr(s1.c_str(),"ffor");
	test_assertSize(s1.length(),4);

	s1.replace(s1.begin() + 1,s1.end(),"foo");
	test_assertStr(s1.c_str(),"ffoo");
	test_assertSize(s1.length(),4);

	s1.replace(s1.begin() + 2,s1.begin() + 3,3,'a');
	test_assertStr(s1.c_str(),"ffaaao");
	test_assertSize(s1.length(),6);

	string s2("bar");
	s1.replace(s1.begin(),s1.end(),s2.begin(),s2.end());
	test_assertStr(s1.c_str(),"bar");
	test_assertSize(s1.length(),3);

	test_caseSucceeded();
}

static void test_find(void) {
	test_caseStart("Testing find()");

	string s1("test");
	test_assertSize(s1.find("test",0),0);
	test_assertSize(s1.find("st",0),2);
	test_assertSize(s1.find("t",0),0);
	test_assertSize(s1.find("t",1),3);
	test_assertSize(s1.find('t',1),3);
	test_assertSize(s1.find(string("t"),1),3);
	test_assertSize(s1.find("foo",0),string::npos);
	test_assertSize(s1.find("test",1),string::npos);
	test_assertSize(s1.find("s",3),string::npos);

	test_caseSucceeded();
}

static void test_rfind(void) {
	test_caseStart("Testing rfind()");

	string s1("test");
	test_assertSize(s1.rfind("test"),0);
	test_assertSize(s1.rfind("st"),2);
	test_assertSize(s1.rfind("t"),3);
	test_assertSize(s1.rfind("t",2),0);
	test_assertSize(s1.rfind('t',2),0);
	test_assertSize(s1.rfind(string("t"),2),0);
	test_assertSize(s1.rfind("foo"),string::npos);
	test_assertSize(s1.rfind("test",2),string::npos);
	test_assertSize(s1.rfind("s",1),string::npos);

	test_caseSucceeded();
}

static void test_find_first_of(void) {
	test_caseStart("Testing find_first_of()");

	string s1("testfoo");
	test_assertSize(s1.find_first_of("test"),0);
	test_assertSize(s1.find_first_of("e"),1);
	test_assertSize(s1.find_first_of("st"),0);
	test_assertSize(s1.find_first_of("foo",0),4);
	test_assertSize(s1.find_first_of("of",0,1),5);
	test_assertSize(s1.find_first_of("of",4),4);
	test_assertSize(s1.find_first_of("of",6),6);
	test_assertSize(s1.find_first_of("abc"),string::npos);
	test_assertSize(s1.find_first_of(""),string::npos);
	test_assertSize(s1.find_first_of("b"),string::npos);

	test_caseSucceeded();
}

static void test_find_last_of(void) {
	test_caseStart("Testing find_last_of()");

	string s1("testfoo");
	test_assertSize(s1.find_last_of("test"),3);
	test_assertSize(s1.find_last_of("e"),1);
	test_assertSize(s1.find_last_of("st"),3);
	test_assertSize(s1.find_last_of("foo"),6);
	test_assertSize(s1.find_last_of("fo",string::npos,1),4);
	test_assertSize(s1.find_last_of("of",4),4);
	test_assertSize(s1.find_last_of("of",6),6);
	test_assertSize(s1.find_last_of("abc"),string::npos);
	test_assertSize(s1.find_last_of(""),string::npos);
	test_assertSize(s1.find_last_of("b"),string::npos);

	test_caseSucceeded();
}

static void test_find_first_not_of(void) {
	test_caseStart("Testing find_first_not_of()");

	string s1("testfoo");
	test_assertSize(s1.find_first_not_of("test"),4);
	test_assertSize(s1.find_first_not_of("testfoo"),string::npos);
	test_assertSize(s1.find_first_not_of("e"),0);
	test_assertSize(s1.find_first_not_of("st"),1);
	test_assertSize(s1.find_first_not_of("foo"),0);
	test_assertSize(s1.find_first_not_of("to",0,1),1);
	test_assertSize(s1.find_first_not_of("of",4),string::npos);
	test_assertSize(s1.find_first_not_of("test",4),4);
	test_assertSize(s1.find_first_not_of("abc"),0);
	test_assertSize(s1.find_first_not_of(""),0);
	test_assertSize(s1.find_first_not_of("b"),0);

	test_caseSucceeded();
}

static void test_find_last_not_of(void) {
	test_caseStart("Testing find_last_not_of()");

	string s1("testfoo");
	test_assertSize(s1.find_last_not_of("test"),6);
	test_assertSize(s1.find_last_not_of("testfoo"),string::npos);
	test_assertSize(s1.find_last_not_of("e"),6);
	test_assertSize(s1.find_last_not_of("st"),6);
	test_assertSize(s1.find_last_not_of("foo"),3);
	test_assertSize(s1.find_last_not_of("to",string::npos,1),6);
	test_assertSize(s1.find_last_not_of("of",4),3);
	test_assertSize(s1.find_last_not_of("test",4),4);
	test_assertSize(s1.find_last_not_of("abc"),6);
	test_assertSize(s1.find_last_not_of(""),6);
	test_assertSize(s1.find_last_not_of("b"),6);

	test_caseSucceeded();
}

static void test_trim(void) {
	test_caseStart("Testing trim()");

	{
		string s("mystring");
		string::size_type count = s.trim();
		test_assertStr(s.c_str(),"mystring");
		test_assertSize(count,0);
	}
	{
		string s(" foo ");
		string::size_type count = s.trim();
		test_assertStr(s.c_str(),"foo");
		test_assertSize(count,2);
	}
	{
		string s("\t\t a");
		string::size_type count = s.trim();
		test_assertStr(s.c_str(),"a");
		test_assertSize(count,3);
	}
	{
		string s("   \t");
		string::size_type count = s.trim();
		test_assertStr(s.c_str(),"");
		test_assertSize(count,4);
	}
	{
		string s("foo    ");
		string::size_type count = s.ltrim();
		test_assertStr(s.c_str(),"foo    ");
		test_assertSize(count,0);
	}
	{
		string s("foo    ");
		string::size_type count = s.rtrim();
		test_assertStr(s.c_str(),"foo");
		test_assertSize(count,4);
	}
	{
		string s("   foo");
		string::size_type count = s.rtrim();
		test_assertStr(s.c_str(),"   foo");
		test_assertSize(count,0);
	}
	{
		string s("   foo");
		string::size_type count = s.ltrim();
		test_assertStr(s.c_str(),"foo");
		test_assertSize(count,3);
	}

	test_caseSucceeded();
}
