/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/mem/paging.h>
#include <sys/video.h>
#include <esc/test.h>
#include "tregion.h"
#include "testutils.h"

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

static void test_region(void) {
	test_1();
	test_2();
	test_3();
	test_4();
}

static void test_1(void) {
	sRegion *reg;
	test_caseStart("Testing reg_create() & reg_destroy()");

	checkMemoryBefore(false);
	reg = reg_create(NULL,124,124,123,PF_DEMANDLOAD,RF_GROWABLE);
	test_assertTrue(reg != NULL);
	test_assertPtr(reg->file,NULL);
	test_assertULInt(reg->flags,RF_GROWABLE);
	test_assertSize(reg->byteCount,124);
	test_assertSize(reg_refCount(reg),0);
	test_assertULInt(reg->pageFlags[0],PF_DEMANDLOAD);
	reg_destroy(reg);
	checkMemoryAfter(false);

	checkMemoryBefore(false);
	reg = reg_create((void*)0x1234,PAGE_SIZE + 1,PAGE_SIZE + 1,124,0,RF_STACK | RF_GROWS_DOWN);
	test_assertTrue(reg != NULL);
	test_assertPtr(reg->file,(void*)0x1234);
	test_assertULInt(reg->flags,RF_STACK | RF_GROWS_DOWN);
	test_assertSize(reg->byteCount,PAGE_SIZE + 1);
	test_assertSize(reg_refCount(reg),0);
	test_assertULInt(reg->pageFlags[0],0);
	test_assertULInt(reg->pageFlags[1],0);
	reg_destroy(reg);
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_2(void) {
	sRegion *reg;
	test_caseStart("Testing reg_addTo() & reg_remFrom()");

	checkMemoryBefore(false);
	reg = reg_create(NULL,124,124,123,PF_DEMANDLOAD,RF_SHAREABLE);
	test_assertTrue(reg != NULL);
	test_assertTrue(reg_addTo(reg,(const void*)0x1234));
	test_assertTrue(reg_addTo(reg,(const void*)0x5678));
	test_assertSize(reg_refCount(reg),2);
	test_assertTrue(reg_remFrom(reg,(const void*)0x5678));
	test_assertSize(reg_refCount(reg),1);
	test_assertTrue(reg_remFrom(reg,(const void*)0x1234));
	test_assertSize(reg_refCount(reg),0);
	reg_destroy(reg);
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_3(void) {
	sRegion *reg;
	size_t i;
	test_caseStart("Testing reg_grow()");

	checkMemoryBefore(false);
	reg = reg_create(NULL,PAGE_SIZE,PAGE_SIZE,123,PF_DEMANDLOAD,RF_GROWABLE);
	test_assertTrue(reg != NULL);
	test_assertSize(reg->byteCount,PAGE_SIZE);
	test_assertULInt(reg->pageFlags[0],PF_DEMANDLOAD);
	test_assertInt(reg_grow(reg,10),0);
	test_assertULInt(reg->pageFlags[0],PF_DEMANDLOAD);
	for(i = 1; i < 11; i++)
		test_assertULInt(reg->pageFlags[i],0);
	test_assertSize(reg->byteCount,PAGE_SIZE * 11);
	test_assertInt(reg_grow(reg,-5),0);
	test_assertSize(reg->byteCount,PAGE_SIZE * 6);
	test_assertInt(reg_grow(reg,-3),0);
	test_assertSize(reg->byteCount,PAGE_SIZE * 3);
	test_assertInt(reg_grow(reg,-3),0);
	test_assertSize(reg->byteCount,0);
	reg_destroy(reg);
	checkMemoryAfter(false);

	checkMemoryBefore(false);
	reg = reg_create(NULL,PAGE_SIZE,0,123,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK | RF_GROWS_DOWN);
	test_assertTrue(reg != NULL);
	test_assertSize(reg->byteCount,PAGE_SIZE);
	test_assertULInt(reg->pageFlags[0],PF_DEMANDLOAD);
	test_assertInt(reg_grow(reg,10),0);
	for(i = 0; i < 10; i++)
		test_assertULInt(reg->pageFlags[i],0);
	test_assertULInt(reg->pageFlags[10],PF_DEMANDLOAD);
	test_assertSize(reg->byteCount,PAGE_SIZE * 11);
	test_assertInt(reg_grow(reg,-5),0);
	for(i = 0; i < 5; i++)
		test_assertULInt(reg->pageFlags[i],0);
	test_assertULInt(reg->pageFlags[5],PF_DEMANDLOAD);
	test_assertSize(reg->byteCount,PAGE_SIZE * 6);
	test_assertInt(reg_grow(reg,-3),0);
	for(i = 0; i < 2; i++)
		test_assertULInt(reg->pageFlags[i],0);
	test_assertULInt(reg->pageFlags[2],PF_DEMANDLOAD);
	test_assertSize(reg->byteCount,PAGE_SIZE * 3);
	test_assertInt(reg_grow(reg,-3),0);
	test_assertSize(reg->byteCount,0);
	reg_destroy(reg);
	checkMemoryAfter(false);

	checkMemoryBefore(false);
	reg = reg_create(NULL,PAGE_SIZE,0,123,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK);
	test_assertTrue(reg != NULL);
	test_assertSize(reg->byteCount,PAGE_SIZE);
	test_assertULInt(reg->pageFlags[0],PF_DEMANDLOAD);
	test_assertInt(reg_grow(reg,10),0);
	for(i = 1; i < 10; i++)
		test_assertULInt(reg->pageFlags[i],0);
	test_assertULInt(reg->pageFlags[0],PF_DEMANDLOAD);
	test_assertSize(reg->byteCount,PAGE_SIZE * 11);
	test_assertInt(reg_grow(reg,-5),0);
	for(i = 1; i < 5; i++)
		test_assertULInt(reg->pageFlags[i],0);
	test_assertULInt(reg->pageFlags[0],PF_DEMANDLOAD);
	test_assertSize(reg->byteCount,PAGE_SIZE * 6);
	test_assertInt(reg_grow(reg,-3),0);
	for(i = 1; i < 2; i++)
		test_assertULInt(reg->pageFlags[i],0);
	test_assertULInt(reg->pageFlags[0],PF_DEMANDLOAD);
	test_assertSize(reg->byteCount,PAGE_SIZE * 3);
	test_assertInt(reg_grow(reg,-3),0);
	test_assertSize(reg->byteCount,0);
	reg_destroy(reg);
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_4(void) {
	sRegion *reg,*clone;
	test_caseStart("Testing reg_clone()");

	checkMemoryBefore(false);
	reg = reg_create((void*)0x1234,PAGE_SIZE,0,123,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK | RF_GROWS_DOWN);
	test_assertTrue(reg != NULL);
	clone = reg_clone((const void*)0x1234,reg);
	test_assertTrue(clone != NULL);
	test_assertPtr(clone->file,(void*)0x1234);
	test_assertULInt(clone->flags,RF_GROWABLE | RF_STACK | RF_GROWS_DOWN);
	test_assertSize(clone->byteCount,PAGE_SIZE);
	test_assertSize(reg_refCount(reg),0);
	test_assertSize(reg_refCount(clone),1);
	test_assertULInt(clone->pageFlags[0],PF_DEMANDLOAD);
	reg_destroy(reg);
	reg_destroy(clone);
	checkMemoryAfter(false);

	test_caseSucceeded();
}
