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
#include <esc/test.h>
#include <sstream>
#include <string>

/* forward declarations */
static void test_streams(void);
static void test_outformat(void);
static void test_informat(void);

/* our test-module */
sTestModule tModStreams = {
	"IOStreams",
	&test_streams
};

static void test_streams(void) {
	test_outformat();
	test_informat();
}

static void test_outformat(void) {
	test_caseStart("Testing output-formatting");

	{
		ostringstream ostr;
		ostr << "foo" << endl;
		ostr << 'b' << 'a' << 'r' << endl;
		ostr << 123 << ' ' << 1234u << ' ' << -12 << ' ' << 0 << endl;
		ostr << 12.3f << ' ' << -2.4f << ' ' << 2.0 << ' ' << 2.0L << endl;
		test_assertStr(ostr.str().c_str(),
				"foo\nbar\n123 1234 -12 0\n12.300000 -2.400000 2.000000 2.000000\n");
	}

	{
		ostringstream ostr;
		ostr.width(5);
		ostr << "foo" << endl;
		ostr.width(2);
		ostr << 'b' << 'a' << 'r' << endl;
		ostr << oct << 0123 << ' ' << hex << showbase << 1234u << ' ' << dec << noshowbase << -12;
		ostr << ' ';
		ostr.width(8);
		ostr.fill('-');
		ostr << left << showpos << 0 << "x" << endl;
		ostr << hex << uppercase << showbase << 0xdeadbeef << endl;
		ostr << true << boolalpha << false << noboolalpha << false << endl;
		ostr.precision(3);
		ostr << 12.3f << ' ' << -2.4L << endl;
		test_assertStr(ostr.str().c_str(),
				"  foo\n bar\n123 0x4d2 -12 +0------x\n0XDEADBEEF\n0X1false0\n+12.300 -2.400\n");
	}

	test_caseSucceeded();
}

static void test_informat(void) {
	test_caseStart("Testing input-formatting");

	{
		const string teststr("123 456 abc 1 false 0 myteststringfoo");
		istringstream istr(teststr);
		char buf[10];
		int i1,i2,i3;
		bool b1,b2,b3;
		istr >> i1;
		istr >> i2;
		istr >> hex >> i3;
		istr >> dec >> b1;
		istr >> boolalpha >> b2;
		istr >> noboolalpha >> b3;
		istr.width(10);
		istr >> buf;
		test_assertInt(i1,123);
		test_assertInt(i2,456);
		test_assertInt(i3,0xabc);
		test_assertTrue(b1);
		test_assertFalse(b2);
		test_assertFalse(b2);
		test_assertStr(buf,"myteststr");
	}

	test_caseSucceeded();
}
