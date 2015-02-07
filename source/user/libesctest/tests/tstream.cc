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

#include <esc/stream/istringstream.h>
#include <esc/stream/ostringstream.h>
#include <esc/stream/std.h>
#include <sys/common.h>
#include <sys/test.h>
#include <math.h>

using namespace esc;

static void test_istream();
static void test_ostream();
static void test_stream();

/* our test-module */
sTestModule tModStream = {
    "Stream",
    &test_stream
};

static void test_stream() {
    test_istream();
    test_ostream();
}

static void test_istream() {
    int a,b;
    uint d;
    float f;
    test_caseStart("Testing IStringStream");

    {
        std::string in("1 2 0xAfd2");
        IStringStream is(in);
        is >> a >> b >> d;
        test_assertInt(a, 1);
        test_assertInt(b, 2);
        test_assertInt(d, 0xAfd2);
    }

    {
        std::string in("  -1\t+2\n\n0XA");
        IStringStream is(in);
        is >> a >> b >> d;
        test_assertInt(a, -1);
        test_assertInt(b, 2);
        test_assertInt(d, 0XA);
    }

    {
        std::string str;
        std::string in("  1\tabc\n\n12.4");
        IStringStream is(in);
        is >> d >> str >> f;
        test_assertInt(d, 1);
        test_assertStr(str.c_str(), "abc");
        test_assertFloat(f, 12.4);
    }

    {
        char buf[16];
        size_t res;
        std::string in(" 1234 55 test\n\n foo \n012345678901234567");
        IStringStream is(in);
        test_assertTrue(is.good());

        res = is.getline(buf, sizeof(buf), '\n');
        test_assertSize(res, 13);
        test_assertStr(buf, " 1234 55 test");

        res = is.getline(buf, sizeof(buf));
        test_assertSize(res, 0);
        test_assertStr(buf, "");

        res = is.getline(buf, sizeof(buf));
        test_assertSize(res, 5);
        test_assertStr(buf, " foo ");

        res = is.getline(buf, sizeof(buf));
        test_assertSize(res, 15);
        test_assertStr(buf, "012345678901234");

        res = is.getline(buf, sizeof(buf));
        test_assertSize(res, 3);
        test_assertStr(buf, "567");

        test_assertTrue(is.eof());
    }

    {
        uint c,d,e;
        std::string in("ABCDEF\n0777\n0666\n0XAB\n0xAB");
        IStringStream is(in);
        is >> fmt(a,"x") >> fmt(b,"o") >> c >> d >> e;
        test_assertInt(a, 0xABCDEF);
        test_assertInt(b, 0777);
        test_assertInt(c, 0666);
        test_assertInt(d, 0xAB);
        test_assertInt(e, 0xAB);
    }

    {
        uintptr_t ptr;
        std::string in;
        if(sizeof(uintptr_t) == 4)
            in = std::string("1234:5678");
        else if(sizeof(uintptr_t) == 8)
            in = std::string("1234:5678:9ABC:DEF0");
        IStringStream is(in);
        is >> fmt(ptr,"p");
        if(sizeof(uintptr_t) == 4)
            test_assertUIntPtr(ptr, 0x12345678);
        else if(sizeof(uintptr_t) == 8)
            test_assertUIntPtr(ptr, 0x123456789ABCDEF0);
    }

    struct TestItem {
        const char *str;
        float res;
    };
    struct TestItem tests[] = {
        {"1234",         1234.f},
        {" 12.34",       12.34f},
        {".5",           .5f},
        {"\t +6.0e2\n",  6.0e2f},
        {"-12.35E5",     -12.35E5f},
    };
    for(size_t i = 0; i < ARRAY_SIZE(tests); i++) {
        std::string in(tests[i].str);
        IStringStream is(in);
        is >> f;
        test_assertFloat(f, tests[i].res);
    }

    test_caseSucceeded();
}

#define STREAM_CHECK(expr, expstr) do {                                                     \
        OStringStream __os;                                                                 \
        __os << expr;                                                                       \
        test_assertStr(__os.str().c_str(), expstr);                                         \
    } while(0)

static void test_ostream() {
    test_caseStart("Testing OStringStream");

    STREAM_CHECK(1 << 2 << 3,
        "123");

    STREAM_CHECK(0x12345678 << "  " << 1.2f << ' ' << '4' << "\n",
        "305419896  1.200 4\n");

    STREAM_CHECK(fmt(1, 2) << ' ' << fmt(123, "0", 10) << ' ' << fmt(0xA23, "#0x", 8),
        " 1 0000000123 0x00000a23");

    STREAM_CHECK(fmt(-123, "+") << ' ' << fmt(123, "+") << ' ' << fmt(444, " ") << ' ' << fmt(-3, " "),
        "-123 +123  444 -3");

    STREAM_CHECK(fmt(-123, "-", 5) << ' ' << fmt(0755, "0o", 5) << ' ' << fmt(0xFF0, "b"),
        "-123  00755 111111110000");

    STREAM_CHECK(fmt(0xDEAD, "#0X", 5) << ' ' << fmt("test", 5, 3) << ' ' << fmt("foo", "-", 4),
        "0X0DEAD   tes foo ");

    OStringStream os;
    os << fmt(0xdeadbeef, "p") << ", " << fmt(0x12345678, "x");
    if(sizeof(uintptr_t) == 4)
        test_assertStr(os.str().c_str(), "dead:beef, 12345678");
    else if(sizeof(uintptr_t) == 8)
        test_assertStr(os.str().c_str(), "0000:0000:dead:beef, 12345678");
    else
        test_assertFalse(true);

    STREAM_CHECK(0.f << ", " << 1.f << ", " << -1.f << ", " << 0.f << ", " << 0.4f << ", " << 18.4f,
                 "0.000, 1.000, -1.000, 0.000, 0.400, 18.399");
    STREAM_CHECK(-1.231f << ", " << 999.999f << ", " << 1234.5678f << ", " << 100189380608.f,
                 "-1.230, 999.999, 1234.567, 100189380608.000");
    STREAM_CHECK(fmt(1., 8, 4) << ", " << fmt(-1., 8, 1) << ", " << fmt(0., 8, 10),
                 "  1.0000,     -1.0, 0.0000000000");
    STREAM_CHECK(fmt(-1.231, 3, 0) << ", " << fmt(999.999, "-", 6, 1)
                                   << ", " << fmt(1234.5678, 2, 4)
                                   << ", " << fmt(1189378123.78167213123, 10, 10),
                 "-1., 999.9 , 1234.5678, 1189378123.7816722393");

    STREAM_CHECK(INFINITY << ", " << -INFINITY << ", " << NAN,
        "inf, -inf, nan");

    test_caseSucceeded();
}
