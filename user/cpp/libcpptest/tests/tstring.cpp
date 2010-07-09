/**
 * $Id: tdir.c 652 2010-05-07 10:58:50Z nasmussen $
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
#include <string>
#include <esc/test.h>

/* forward declarations */
static void test_string(void);
static void test_constr(void);
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

/* our test-module */
sTestModule tModString = {
	"String",
	&test_string
};

static void test_string(void) {
	test_constr();
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
}

static void test_constr(void) {
	test_caseStart("Testing constructors");

	string s1;
	test_assertStr(s1.c_str(),"");
	test_assertUInt(s1.length(),0);

	string s1_("mystring");
	test_assertStr(s1_.c_str(),"mystring");
	test_assertUInt(s1_.length(),8);

	string s2(s1);
	test_assertStr(s2.c_str(),"");
	test_assertUInt(s2.length(),0);

	s2.append("test");
	string s3(s2);
	test_assertStr(s2.c_str(),"test");
	test_assertUInt(s2.length(),4);

	string s4(s3,0,4);
	test_assertStr(s4.c_str(),"test");
	test_assertUInt(s4.length(),4);
	string s5(s3,1,3);
	test_assertStr(s5.c_str(),"est");
	test_assertUInt(s5.length(),3);
	string s6(s3,2,1);
	test_assertStr(s6.c_str(),"s");
	test_assertUInt(s6.length(),1);
	string s7(s3,3,0);
	test_assertStr(s7.c_str(),"");
	test_assertUInt(s7.length(),0);

	string s8("foo",1);
	test_assertStr(s8.c_str(),"f");
	test_assertUInt(s8.length(),1);
	string s9("bar",3);
	test_assertStr(s9.c_str(),"bar");
	test_assertUInt(s9.length(),3);
	string s10("bla",0);
	test_assertStr(s10.c_str(),"");
	test_assertUInt(s10.length(),0);

	string s11(0,'c');
	test_assertStr(s11.c_str(),"");
	test_assertUInt(s11.length(),0);
	string s12(1,'c');
	test_assertStr(s12.c_str(),"c");
	test_assertUInt(s12.length(),1);
	string s13(12,'a');
	test_assertStr(s13.c_str(),"aaaaaaaaaaaa");
	test_assertUInt(s13.length(),12);

	test_caseSucceded();
}

static void test_assign(void) {
	test_caseStart("Testing assign-operator");

	string s1("test");
	string s1c = s1;
	test_assertStr(s1c.c_str(),"test");
	test_assertUInt(s1c.length(),4);

	string s2("foo");
	s2 = s1;
	test_assertStr(s2.c_str(),"test");
	test_assertUInt(s2.length(),4);

	s2 = "mystr";
	test_assertStr(s2.c_str(),"mystr");
	test_assertUInt(s2.length(),5);

	s2 = 'a';
	test_assertStr(s2.c_str(),"a");
	test_assertUInt(s2.length(),1);

	test_caseSucceded();
}

static void test_resize(void) {
	test_caseStart("Testing resize() and reserve()");

	string s1("abc");
	s1.resize(3);
	test_assertStr(s1.c_str(),"abc");
	test_assertUInt(s1.length(),3);
	test_assertUInt(s1.max_size(),3);

	s1.resize(2);
	test_assertStr(s1.c_str(),"ab");
	test_assertUInt(s1.length(),2);
	test_assertUInt(s1.max_size(),2);

	s1.resize(0);
	test_assertStr(s1.c_str(),"");
	test_assertUInt(s1.length(),0);
	test_assertUInt(s1.max_size(),0);

	s1.resize(12);
	test_assertStr(s1.c_str(),"");
	test_assertUInt(s1.length(),0);
	test_assertUInt(s1.max_size(),12);

	s1.reserve(16);
	test_assertTrue(s1.max_size() >= 16);

	s1.reserve(12);
	test_assertTrue(s1.max_size() >= 12);

	test_caseSucceded();
}

static void test_clear(void) {
	test_caseStart("Testing clear()");

	string s1("abc");
	s1.clear();
	test_assertStr(s1.c_str(),"");
	test_assertUInt(s1.length(),0);

	test_caseSucceded();
}

static void test_append(void) {
	test_caseStart("Testing append()");

	string s1;
	s1.append("test");
	test_assertStr(s1.c_str(),"test");
	test_assertUInt(s1.length(),4);
	s1.append("foo");
	test_assertStr(s1.c_str(),"testfoo");
	test_assertUInt(s1.length(),7);
	s1.append("bar");
	test_assertStr(s1.c_str(),"testfoobar");
	test_assertUInt(s1.length(),10);

	string s2("abc");
	s2.append(s1);
	test_assertStr(s2.c_str(),"abctestfoobar");
	test_assertUInt(s2.length(),13);
	s2.append(s1,0,3);
	test_assertStr(s2.c_str(),"abctestfoobartes");
	test_assertUInt(s2.length(),16);
	s2.append(s1,3,5);
	test_assertStr(s2.c_str(),"abctestfoobartestfoob");
	test_assertUInt(s2.length(),21);
	s2.append(s1,s1.length() - 1,1);
	test_assertStr(s2.c_str(),"abctestfoobartestfoobr");
	test_assertUInt(s2.length(),22);

	string s3;
	s3.append(10,'c');
	test_assertStr(s3.c_str(),"cccccccccc");
	test_assertUInt(s3.length(),10);

	string s4;
	s3.append(0,'a');
	test_assertStr(s4.c_str(),"");
	test_assertUInt(s4.length(),0);

	test_caseSucceded();
}

static void test_insert(void) {
	test_caseStart("Testing insert()");

	string s1("test");
	s1.insert(1,"abc");
	test_assertStr(s1.c_str(),"tabcest");
	test_assertUInt(s1.length(),7);

	s1.insert(0,"a");
	test_assertStr(s1.c_str(),"atabcest");
	test_assertUInt(s1.length(),8);

	s1.insert(4,string("a"));
	test_assertStr(s1.c_str(),"atabacest");
	test_assertUInt(s1.length(),9);

	s1.insert(s1.length(),string("foo"),1,1);
	test_assertStr(s1.c_str(),"atabacesto");
	test_assertUInt(s1.length(),10);

	test_caseSucceded();
}

static void test_erase(void) {
	test_caseStart("Testing erase()");

	string s1("test");
	s1.erase();
	test_assertStr(s1.c_str(),"");
	test_assertUInt(s1.length(),0);

	string s2("test");
	s2.erase(0,1);
	test_assertStr(s2.c_str(),"est");
	test_assertUInt(s2.length(),3);
	s2.erase(1,1);
	test_assertStr(s2.c_str(),"et");
	test_assertUInt(s2.length(),2);
	s2.erase(0,2);
	test_assertStr(s2.c_str(),"");
	test_assertUInt(s2.length(),0);

	string s3("foobar");
	s3.erase(3,3);
	test_assertStr(s3.c_str(),"foo");
	test_assertUInt(s3.length(),3);
	s3.erase(2,2);
	test_assertStr(s3.c_str(),"fo");
	test_assertUInt(s3.length(),2);

	test_caseSucceded();
}

static void test_replace(void) {
	test_caseStart("Testing replace()");

	string s1("test");
	s1.replace(0,2,"foo");
	test_assertStr(s1.c_str(),"foost");
	test_assertUInt(s1.length(),5);

	s1.replace(0,0,"bar");
	test_assertStr(s1.c_str(),"barfoost");
	test_assertUInt(s1.length(),8);

	s1.replace(7,1,"test");
	test_assertStr(s1.c_str(),"barfoostest");
	test_assertUInt(s1.length(),11);

	s1.replace(11,12,"a");
	test_assertStr(s1.c_str(),"barfoostesta");
	test_assertUInt(s1.length(),12);

	test_caseSucceded();
}

static void test_find(void) {
	test_caseStart("Testing find()");

	string s1("test");
	test_assertUInt(s1.find("test",0),0);
	test_assertUInt(s1.find("st",0),2);
	test_assertUInt(s1.find("t",0),0);
	test_assertUInt(s1.find("t",1),3);
	test_assertUInt(s1.find('t',1),3);
	test_assertUInt(s1.find(string("t"),1),3);
	test_assertUInt(s1.find("foo",0),string::npos);
	test_assertUInt(s1.find("test",1),string::npos);
	test_assertUInt(s1.find("s",3),string::npos);

	test_caseSucceded();
}

static void test_rfind(void) {
	test_caseStart("Testing rfind()");

	string s1("test");
	test_assertUInt(s1.rfind("test"),0);
	test_assertUInt(s1.rfind("st"),2);
	test_assertUInt(s1.rfind("t"),3);
	test_assertUInt(s1.rfind("t",2),0);
	test_assertUInt(s1.rfind('t',2),0);
	test_assertUInt(s1.rfind(string("t"),2),0);
	test_assertUInt(s1.rfind("foo"),string::npos);
	test_assertUInt(s1.rfind("test",2),string::npos);
	test_assertUInt(s1.rfind("s",1),string::npos);

	test_caseSucceded();
}

static void test_find_first_of(void) {
	test_caseStart("Testing find_first_of()");

	string s1("testfoo");
	test_assertUInt(s1.find_first_of("test"),0);
	test_assertUInt(s1.find_first_of("e"),1);
	test_assertUInt(s1.find_first_of("st"),0);
	test_assertUInt(s1.find_first_of("foo",0),4);
	test_assertUInt(s1.find_first_of("of",0,1),5);
	test_assertUInt(s1.find_first_of("of",4),4);
	test_assertUInt(s1.find_first_of("of",6),6);
	test_assertUInt(s1.find_first_of("abc"),string::npos);
	test_assertUInt(s1.find_first_of(""),string::npos);
	test_assertUInt(s1.find_first_of("b"),string::npos);

	test_caseSucceded();
}

static void test_find_last_of(void) {
	test_caseStart("Testing find_last_of()");

	string s1("testfoo");
	test_assertUInt(s1.find_last_of("test"),3);
	test_assertUInt(s1.find_last_of("e"),1);
	test_assertUInt(s1.find_last_of("st"),3);
	test_assertUInt(s1.find_last_of("foo"),6);
	test_assertUInt(s1.find_last_of("fo",string::npos,1),4);
	test_assertUInt(s1.find_last_of("of",4),4);
	test_assertUInt(s1.find_last_of("of",6),6);
	test_assertUInt(s1.find_last_of("abc"),string::npos);
	test_assertUInt(s1.find_last_of(""),string::npos);
	test_assertUInt(s1.find_last_of("b"),string::npos);

	test_caseSucceded();
}

static void test_find_first_not_of(void) {
	test_caseStart("Testing find_first_not_of()");

	string s1("testfoo");
	test_assertUInt(s1.find_first_not_of("test"),4);
	test_assertUInt(s1.find_first_not_of("testfoo"),string::npos);
	test_assertUInt(s1.find_first_not_of("e"),0);
	test_assertUInt(s1.find_first_not_of("st"),1);
	test_assertUInt(s1.find_first_not_of("foo"),0);
	test_assertUInt(s1.find_first_not_of("to",0,1),1);
	test_assertUInt(s1.find_first_not_of("of",4),string::npos);
	test_assertUInt(s1.find_first_not_of("test",4),4);
	test_assertUInt(s1.find_first_not_of("abc"),0);
	test_assertUInt(s1.find_first_not_of(""),0);
	test_assertUInt(s1.find_first_not_of("b"),0);

	test_caseSucceded();
}

static void test_find_last_not_of(void) {
	test_caseStart("Testing find_last_not_of()");

	string s1("testfoo");
	test_assertUInt(s1.find_last_not_of("test"),6);
	test_assertUInt(s1.find_last_not_of("testfoo"),string::npos);
	test_assertUInt(s1.find_last_not_of("e"),6);
	test_assertUInt(s1.find_last_not_of("st"),6);
	test_assertUInt(s1.find_last_not_of("foo"),3);
	test_assertUInt(s1.find_last_not_of("to",string::npos,1),6);
	test_assertUInt(s1.find_last_not_of("of",4),3);
	test_assertUInt(s1.find_last_not_of("test",4),4);
	test_assertUInt(s1.find_last_not_of("abc"),6);
	test_assertUInt(s1.find_last_not_of(""),6);
	test_assertUInt(s1.find_last_not_of("b"),6);

	test_caseSucceded();
}
