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
#include <esc/ringbuffer.h>
#include <stdlib.h>

/* forward declarations */
static void test_rbuf();
static void test_1();
static void test_2();
static void test_3();
static void test_4();
static void test_5();

/* our test-module */
sTestModule tModRBuffer = {
	"Ring buffer",
	&test_rbuf
};

static void test_rbuf() {
	test_1();
	test_2();
	test_3();
	test_4();
	test_5();
}

static void test_1() {
	size_t oldFree;

	test_caseStart("Create & Destroy");
	oldFree = heapspace();

	{
		esc::RingBuffer<size_t> rb(10);
		test_assertSize(rb.length(),0);
	}

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_2() {
	size_t oldFree;
	test_caseStart("Write & Read");
	oldFree = heapspace();

	{
		esc::RingBuffer<size_t> rb(10,esc::RB_DEFAULT);

		for(size_t i = 0; i < 10; i++)
			test_assertTrue(rb.write(i));
		test_assertSize(rb.length(),10);
		for(size_t i = 0; i < 10; i++) {
			size_t x = 0;
			test_assertTrue(rb.read(&x));
			test_assertSize(x,i);
		}
		test_assertSize(rb.length(),0);
	}

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_3() {
	size_t oldFree;
	test_caseStart("Write & Read - Full RB_OVERWRITE");
	oldFree = heapspace();

	{
		esc::RingBuffer<size_t> rb(5,esc::RB_OVERWRITE);

		/* overwrite 1 */
		for(size_t i = 0; i < 6; i++)
			test_assertTrue(rb.write(i));
		test_assertSize(rb.length(),5);
		for(size_t i = 0; i < 5; i++) {
			size_t x = 0;
			test_assertTrue(rb.read(&x));
			test_assertSize(x,i + 1);
		}
		test_assertSize(rb.length(),0);

		/* overwrite 5 = all */
		for(size_t i = 0; i < 10; i++)
			test_assertTrue(rb.write(i));
		test_assertSize(rb.length(),5);
		for(size_t i = 0; i < 5; i++) {
			size_t x = 0;
			test_assertTrue(rb.read(&x));
			test_assertSize(x,i + 5);
		}
		test_assertSize(rb.length(),0);

		/* overwrite 2 */
		for(size_t i = 0; i < 7; i++)
			test_assertTrue(rb.write(i));
		test_assertSize(rb.length(),5);
		for(size_t i = 0; i < 5; i++) {
			size_t x = 0;
			test_assertTrue(rb.read(&x));
			test_assertSize(x,i + 2);
		}
		test_assertSize(rb.length(),0);
	}

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_4() {
	size_t oldFree;
	test_caseStart("Write & Read - Full RB_DEFAULT");
	oldFree = heapspace();

	{
		esc::RingBuffer<size_t> rb(5,esc::RB_DEFAULT);

		size_t i;
		for(i = 0; i < 5; i++)
			test_assertTrue(rb.write(i));
		test_assertSize(rb.length(),5);
		test_assertFalse(rb.write(i));
		test_assertSize(rb.length(),5);
		for(i = 0; i < 5; i++) {
			size_t x = 0;
			test_assertTrue(rb.read(&x));
			test_assertSize(x,i);
		}
		test_assertSize(rb.length(),0);
	}

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}

static void test_5() {
	size_t oldFree;
	test_caseStart("Move");
	oldFree = heapspace();

	{
		esc::RingBuffer<size_t> rb1(8,esc::RB_OVERWRITE);
		esc::RingBuffer<size_t> rb2(5,esc::RB_OVERWRITE);

		for(size_t i = 0; i < 5; i++)
			test_assertTrue(rb1.write(i));
		test_assertSize(rb2.move(rb1,5),5);
		test_assertSize(rb1.length(),0);
		test_assertSize(rb2.length(),5);

		test_assertSize(rb1.move(rb2,3),3);
		test_assertSize(rb1.length(),3);
		test_assertSize(rb2.length(),2);

		test_assertSize(rb1.move(rb2,1),1);
		test_assertSize(rb1.length(),4);
		test_assertSize(rb2.length(),1);

		test_assertSize(rb1.move(rb2,4),1);
		test_assertSize(rb1.length(),5);
		test_assertSize(rb2.length(),0);

		test_assertSize(rb2.move(rb1,5),5);
		test_assertSize(rb1.length(),0);
		test_assertSize(rb2.length(),5);
	}

	test_assertSize(heapspace(),oldFree);
	test_caseSucceeded();
}
