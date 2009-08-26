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

#include <common.h>
#include <kheap.h>
#include <video.h>
#include <ringbuffer.h>
#include <test.h>
#include "tringbuffer.h"

/* forward declarations */
static void test_rbuf(void);
static void test_1(void);
static void test_2(void);
static void test_3(void);
static void test_4(void);
static void test_5(void);

/* our test-module */
sTestModule tModRBuffer = {
	"Ring buffer",
	&test_rbuf
};

static void test_rbuf(void) {
	test_1();
	test_2();
	test_3();
	test_4();
	test_5();
}

static void test_1(void) {
	sRingBuf *rb;
	u32 free = kheap_getFreeMem();
	test_caseStart("Create & Destroy");

	rb = rb_create(sizeof(u32),10,RB_DEFAULT);
	test_assertTrue(rb != NULL);

	test_assertUInt(rb_length(rb),0);

	rb_destroy(rb);

	if(kheap_getFreeMem() != free)
		test_caseFailed("Memory not freed");
	else
		test_caseSucceded();
}

static void test_2(void) {
	sRingBuf *rb;
	u32 i,x;
	test_caseStart("Write & Read");

	rb = rb_create(sizeof(u32),10,RB_DEFAULT);
	test_assertTrue(rb != NULL);

	for(i = 0; i < 10; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertUInt(rb_length(rb),10);
	for(i = 0; i < 10; i++) {
		test_assertTrue(rb_read(rb,&x));
		test_assertUInt(x,i);
	}
	test_assertUInt(rb_length(rb),0);

	rb_destroy(rb);

	test_caseSucceded();
}

static void test_3(void) {
	sRingBuf *rb;
	u32 i,x;
	test_caseStart("Write & Read - Full RB_OVERWRITE");

	rb = rb_create(sizeof(u32),5,RB_OVERWRITE);
	test_assertTrue(rb != NULL);

	/* overwrite 1 */
	for(i = 0; i < 6; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertUInt(rb_length(rb),5);
	for(i = 0; i < 5; i++) {
		test_assertTrue(rb_read(rb,&x));
		test_assertUInt(x,i + 1);
	}
	test_assertUInt(rb_length(rb),0);

	/* overwrite 5 = all */
	for(i = 0; i < 10; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertUInt(rb_length(rb),5);
	for(i = 0; i < 5; i++) {
		test_assertTrue(rb_read(rb,&x));
		test_assertUInt(x,i + 5);
	}
	test_assertUInt(rb_length(rb),0);

	/* overwrite 2 */
	for(i = 0; i < 7; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertUInt(rb_length(rb),5);
	for(i = 0; i < 5; i++) {
		test_assertTrue(rb_read(rb,&x));
		test_assertUInt(x,i + 2);
	}
	test_assertUInt(rb_length(rb),0);

	rb_destroy(rb);

	test_caseSucceded();
}

static void test_4(void) {
	sRingBuf *rb;
	u32 i,x;
	test_caseStart("Write & Read - Full RB_DEFAULT");

	rb = rb_create(sizeof(u32),5,RB_DEFAULT);
	test_assertTrue(rb != NULL);

	for(i = 0; i < 5; i++)
		test_assertTrue(rb_write(rb,&i));
	test_assertUInt(rb_length(rb),5);
	test_assertFalse(rb_write(rb,&i));
	test_assertUInt(rb_length(rb),5);
	for(i = 0; i < 5; i++) {
		test_assertTrue(rb_read(rb,&x));
		test_assertUInt(x,i);
	}
	test_assertUInt(rb_length(rb),0);

	rb_destroy(rb);

	test_caseSucceded();
}

static void test_5(void) {
	u32 i;
	sRingBuf *rb1,*rb2;
	test_caseStart("Move");

	rb1 = rb_create(sizeof(u32),8,RB_OVERWRITE);
	rb2 = rb_create(sizeof(u32),5,RB_OVERWRITE);
	test_assertTrue(rb1 != NULL);
	test_assertTrue(rb2 != NULL);

	for(i = 0; i < 5; i++)
		test_assertTrue(rb_write(rb1,&i));
	test_assertUInt(rb_move(rb2,rb1,5),5);
	test_assertUInt(rb_length(rb1),0);
	test_assertUInt(rb_length(rb2),5);

	test_assertUInt(rb_move(rb1,rb2,3),3);
	test_assertUInt(rb_length(rb1),3);
	test_assertUInt(rb_length(rb2),2);

	test_assertUInt(rb_move(rb1,rb2,1),1);
	test_assertUInt(rb_length(rb1),4);
	test_assertUInt(rb_length(rb2),1);

	test_assertUInt(rb_move(rb1,rb2,4),1);
	test_assertUInt(rb_length(rb1),5);
	test_assertUInt(rb_length(rb2),0);

	test_assertUInt(rb_move(rb2,rb1,5),5);
	test_assertUInt(rb_length(rb1),0);
	test_assertUInt(rb_length(rb2),5);

	rb_destroy(rb1);
	rb_destroy(rb2);

	test_caseSucceded();
}
