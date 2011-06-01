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

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/mem/region.h>
#include <sys/video.h>
#include <esc/test.h>
#include "tregion.h"

/* forward declarations */
static void test_region(void);
static void test_1(void);
static void test_2(void);
static void test_3(void);
static void test_4(void);

/* our test-module */
sTestModule tModRegion = {
	"VMM Regions",
	&test_region
};
static size_t heapBefore;
static size_t heapAfter;

static void test_init(void) {
	heapBefore = kheap_getFreeMem();
}
static void test_finish(void) {
	heapAfter = kheap_getFreeMem();
	test_assertUInt(heapAfter,heapBefore);
}

static void test_region(void) {
	test_1();
	test_2();
	test_3();
	test_4();
}

static void test_1(void) {
	sRegion *reg;
	sBinDesc bindesc;
	test_caseStart("Testing reg_create() & reg_destroy()");

	test_init();
	reg = reg_create(NULL,123,124,124,PF_DEMANDLOAD,RF_GROWABLE);
	test_assertTrue(reg != NULL);
	test_assertUInt(reg->binary.modifytime,0);
	test_assertInt(reg->binary.ino,0);
	test_assertInt(reg->binary.dev,0);
	test_assertUInt(reg->binOffset,0);
	test_assertUInt(reg->flags,RF_GROWABLE);
	test_assertUInt(reg->byteCount,124);
	test_assertUInt(reg_refCount(reg),0);
	test_assertUInt(reg->pageFlags[0],PF_DEMANDLOAD);
	reg_destroy(reg);
	test_finish();

	test_init();
	bindesc.modifytime = 1234;
	bindesc.ino = 42;
	bindesc.dev = 4;
	reg = reg_create(&bindesc,123,4097,4097,0,RF_STACK);
	test_assertTrue(reg != NULL);
	test_assertUInt(reg->binary.modifytime,1234);
	test_assertInt(reg->binary.ino,42);
	test_assertInt(reg->binary.dev,4);
	test_assertUInt(reg->binOffset,123);
	test_assertUInt(reg->flags,RF_STACK);
	test_assertUInt(reg->byteCount,4097);
	test_assertUInt(reg_refCount(reg),0);
	test_assertUInt(reg->pageFlags[0],0);
	test_assertUInt(reg->pageFlags[1],0);
	reg_destroy(reg);
	test_finish();

	test_caseSucceeded();
}

static void test_2(void) {
	sRegion *reg;
	test_caseStart("Testing reg_addTo() & reg_remFrom()");

	test_init();
	reg = reg_create(NULL,123,124,124,PF_DEMANDLOAD,RF_SHAREABLE);
	test_assertTrue(reg != NULL);
	test_assertTrue(reg_addTo(reg,(const void*)0x1234));
	test_assertTrue(reg_addTo(reg,(const void*)0x5678));
	test_assertUInt(reg_refCount(reg),2);
	test_assertTrue(reg_remFrom(reg,(const void*)0x5678));
	test_assertUInt(reg_refCount(reg),1);
	test_assertTrue(reg_remFrom(reg,(const void*)0x1234));
	test_assertUInt(reg_refCount(reg),0);
	reg_destroy(reg);
	test_finish();

	test_caseSucceeded();
}

static void test_3(void) {
	sRegion *reg;
	size_t i;
	test_caseStart("Testing reg_grow()");

	test_init();
	reg = reg_create(NULL,123,0x1000,0x1000,PF_DEMANDLOAD,RF_GROWABLE);
	test_assertTrue(reg != NULL);
	test_assertUInt(reg->byteCount,0x1000);
	test_assertUInt(reg->pageFlags[0],PF_DEMANDLOAD);
	test_assertTrue(reg_grow(reg,10));
	test_assertUInt(reg->pageFlags[0],PF_DEMANDLOAD);
	for(i = 1; i < 11; i++)
		test_assertUInt(reg->pageFlags[i],0);
	test_assertUInt(reg->byteCount,0xB000);
	test_assertTrue(reg_grow(reg,-5));
	test_assertUInt(reg->byteCount,0x6000);
	test_assertTrue(reg_grow(reg,-3));
	test_assertUInt(reg->byteCount,0x3000);
	test_assertTrue(reg_grow(reg,-3));
	test_assertUInt(reg->byteCount,0x0000);
	reg_destroy(reg);
	test_finish();

	test_init();
	reg = reg_create(NULL,123,0x1000,0,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK);
	test_assertTrue(reg != NULL);
	test_assertUInt(reg->byteCount,0x1000);
	test_assertUInt(reg->pageFlags[0],PF_DEMANDLOAD);
	test_assertTrue(reg_grow(reg,10));
	for(i = 0; i < 10; i++)
		test_assertUInt(reg->pageFlags[i],0);
	test_assertUInt(reg->pageFlags[10],PF_DEMANDLOAD);
	test_assertUInt(reg->byteCount,0xB000);
	test_assertTrue(reg_grow(reg,-5));
	for(i = 0; i < 5; i++)
		test_assertUInt(reg->pageFlags[i],0);
	test_assertUInt(reg->pageFlags[5],PF_DEMANDLOAD);
	test_assertUInt(reg->byteCount,0x6000);
	test_assertTrue(reg_grow(reg,-3));
	for(i = 0; i < 2; i++)
		test_assertUInt(reg->pageFlags[i],0);
	test_assertUInt(reg->pageFlags[2],PF_DEMANDLOAD);
	test_assertUInt(reg->byteCount,0x3000);
	test_assertTrue(reg_grow(reg,-3));
	test_assertUInt(reg->byteCount,0x0000);
	reg_destroy(reg);
	test_finish();

	test_caseSucceeded();
}

static void test_4(void) {
	sRegion *reg,*clone;
	sBinDesc bindesc;
	test_caseStart("Testing reg_clone()");

	test_init();
	bindesc.modifytime = 444;
	bindesc.ino = 23;
	bindesc.dev = 2;
	reg = reg_create(&bindesc,123,0x1000,0,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK);
	test_assertTrue(reg != NULL);
	clone = reg_clone((const void*)0x1234,reg);
	test_assertTrue(clone != NULL);
	test_assertUInt(clone->binary.modifytime,444);
	test_assertInt(clone->binary.ino,23);
	test_assertInt(clone->binary.dev,2);
	test_assertUInt(clone->binOffset,123);
	test_assertUInt(clone->flags,RF_GROWABLE | RF_STACK);
	test_assertUInt(clone->byteCount,0x1000);
	test_assertUInt(reg_refCount(reg),0);
	test_assertUInt(reg_refCount(clone),1);
	test_assertUInt(clone->pageFlags[0],PF_DEMANDLOAD);
	reg_destroy(reg);
	reg_destroy(clone);
	test_finish();

	test_caseSucceeded();
}
