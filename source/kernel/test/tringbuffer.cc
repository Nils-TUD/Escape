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
#include <sys/mem/kheap.h>
#include <sys/video.h>
#include <esc/ringbuffer.h>
#include <esc/test.h>
#include "testutils.h"

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
	sRingBuf *rb;
	test_caseStart("Create & Destroy");
	checkMemoryBefore(false);

	rb = rb_create(sizeof(size_t),10,RB_DEFAULT);
	test_assertTrue(rb != NULL);

	test_assertSize(rb_length(rb),0);

	rb_destroy(rb);

	checkMemoryAfter(false);
	test_caseSucceeded();
}

static void test_2() {
	sRingBuf *rb;
	test_caseStart("Write & Read");
	checkMemoryBefore(false);

	rb = rb_create(sizeof(size_t),10,RB_DEFAULT);
	test_assertTrue(rb != NULL);

	for(size_t i = 0; i < 10; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertSize(rb_length(rb),10);
	for(size_t i = 0; i < 10; i++) {
		size_t x;
		test_assertTrue(rb_read(rb,&x));
		test_assertSize(x,i);
	}
	test_assertSize(rb_length(rb),0);

	rb_destroy(rb);

	checkMemoryAfter(false);
	test_caseSucceeded();
}

static void test_3() {
	sRingBuf *rb;
	test_caseStart("Write & Read - Full RB_OVERWRITE");
	checkMemoryBefore(false);

	rb = rb_create(sizeof(size_t),5,RB_OVERWRITE);
	test_assertTrue(rb != NULL);

	/* overwrite 1 */
	for(size_t i = 0; i < 6; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertSize(rb_length(rb),5);
	for(size_t i = 0; i < 5; i++) {
		size_t x;
		test_assertTrue(rb_read(rb,&x));
		test_assertSize(x,i + 1);
	}
	test_assertSize(rb_length(rb),0);

	/* overwrite 5 = all */
	for(size_t i = 0; i < 10; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertSize(rb_length(rb),5);
	for(size_t i = 0; i < 5; i++) {
		size_t x;
		test_assertTrue(rb_read(rb,&x));
		test_assertSize(x,i + 5);
	}
	test_assertSize(rb_length(rb),0);

	/* overwrite 2 */
	for(size_t i = 0; i < 7; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertSize(rb_length(rb),5);
	for(size_t i = 0; i < 5; i++) {
		size_t x;
		test_assertTrue(rb_read(rb,&x));
		test_assertSize(x,i + 2);
	}
	test_assertSize(rb_length(rb),0);

	rb_destroy(rb);

	checkMemoryAfter(false);
	test_caseSucceeded();
}

static void test_4() {
	sRingBuf *rb;
	test_caseStart("Write & Read - Full RB_DEFAULT");
	checkMemoryBefore(false);

	rb = rb_create(sizeof(size_t),5,RB_DEFAULT);
	test_assertTrue(rb != NULL);

	size_t i;
	for(i = 0; i < 5; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertSize(rb_length(rb),5);
	test_assertFalse(rb_write(rb,&i));
	test_assertSize(rb_length(rb),5);
	for(i = 0; i < 5; i++) {
		size_t x;
		test_assertTrue(rb_read(rb,&x));
		test_assertSize(x,i);
	}
	test_assertSize(rb_length(rb),0);

	rb_destroy(rb);

	checkMemoryAfter(false);
	test_caseSucceeded();
}

static void test_5() {
	sRingBuf *rb1,*rb2;
	test_caseStart("Move");
	checkMemoryBefore(false);

	rb1 = rb_create(sizeof(size_t),8,RB_OVERWRITE);
	rb2 = rb_create(sizeof(size_t),5,RB_OVERWRITE);
	test_assertTrue(rb1 != NULL);
	test_assertTrue(rb2 != NULL);

	for(size_t i = 0; i < 5; i++)
		test_assertTrue(rb_write(rb1,&i));
	test_assertSize(rb_move(rb2,rb1,5),5);
	test_assertSize(rb_length(rb1),0);
	test_assertSize(rb_length(rb2),5);

	test_assertSize(rb_move(rb1,rb2,3),3);
	test_assertSize(rb_length(rb1),3);
	test_assertSize(rb_length(rb2),2);

	test_assertSize(rb_move(rb1,rb2,1),1);
	test_assertSize(rb_length(rb1),4);
	test_assertSize(rb_length(rb2),1);

	test_assertSize(rb_move(rb1,rb2,4),1);
	test_assertSize(rb_length(rb1),5);
	test_assertSize(rb_length(rb2),0);

	test_assertSize(rb_move(rb2,rb1,5),5);
	test_assertSize(rb_length(rb1),0);
	test_assertSize(rb_length(rb2),5);

	rb_destroy(rb1);
	rb_destroy(rb2);

	checkMemoryAfter(false);
	test_caseSucceeded();
}
