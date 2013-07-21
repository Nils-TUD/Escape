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
	Region *reg;
	test_caseStart("Testing Region::create() & Region::destroy()");

	checkMemoryBefore(false);
	reg = Region::create(NULL,124,124,123,PF_DEMANDLOAD,RF_GROWABLE);
	test_assertTrue(reg != NULL);
	test_assertPtr(reg->getFile(),NULL);
	test_assertULInt(reg->getFlags(),RF_GROWABLE);
	test_assertSize(reg->getByteCount(),124);
	test_assertSize(reg->refCount(),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	reg->destroy();
	checkMemoryAfter(false);

	checkMemoryBefore(false);
	reg = Region::create((sFile*)0x1234,PAGE_SIZE + 1,PAGE_SIZE + 1,124,0,RF_STACK | RF_GROWS_DOWN);
	test_assertTrue(reg != NULL);
	test_assertPtr(reg->getFile(),(void*)0x1234);
	test_assertULInt(reg->getFlags(),RF_STACK | RF_GROWS_DOWN);
	test_assertSize(reg->getByteCount(),PAGE_SIZE + 1);
	test_assertSize(reg->refCount(),0);
	test_assertULInt(reg->getPageFlags(0),0);
	test_assertULInt(reg->getPageFlags(1),0);
	reg->destroy();
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_2(void) {
	Region *reg;
	test_caseStart("Testing Region::addTo() & Region::remFrom()");

	checkMemoryBefore(false);
	reg = Region::create(NULL,124,124,123,PF_DEMANDLOAD,RF_SHAREABLE);
	test_assertTrue(reg != NULL);
	test_assertTrue(reg->addTo((const void*)0x1234));
	test_assertTrue(reg->addTo((const void*)0x5678));
	test_assertSize(reg->refCount(),2);
	test_assertTrue(reg->remFrom((const void*)0x5678));
	test_assertSize(reg->refCount(),1);
	test_assertTrue(reg->remFrom((const void*)0x1234));
	test_assertSize(reg->refCount(),0);
	reg->destroy();
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_3(void) {
	Region *reg;
	size_t i;
	test_caseStart("Testing Region::grow()");

	checkMemoryBefore(false);
	reg = Region::create(NULL,PAGE_SIZE,PAGE_SIZE,123,PF_DEMANDLOAD,RF_GROWABLE);
	test_assertTrue(reg != NULL);
	test_assertSize(reg->getByteCount(),PAGE_SIZE);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertInt(reg->grow(10),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	for(i = 1; i < 11; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 11);
	test_assertInt(reg->grow(-5),0);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 6);
	test_assertInt(reg->grow(-3),0);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 3);
	test_assertInt(reg->grow(-3),0);
	test_assertSize(reg->getByteCount(),0);
	reg->destroy();
	checkMemoryAfter(false);

	checkMemoryBefore(false);
	reg = Region::create(NULL,PAGE_SIZE,0,123,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK | RF_GROWS_DOWN);
	test_assertTrue(reg != NULL);
	test_assertSize(reg->getByteCount(),PAGE_SIZE);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertInt(reg->grow(10),0);
	for(i = 0; i < 10; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(10),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 11);
	test_assertInt(reg->grow(-5),0);
	for(i = 0; i < 5; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(5),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 6);
	test_assertInt(reg->grow(-3),0);
	for(i = 0; i < 2; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(2),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 3);
	test_assertInt(reg->grow(-3),0);
	test_assertSize(reg->getByteCount(),0);
	reg->destroy();
	checkMemoryAfter(false);

	checkMemoryBefore(false);
	reg = Region::create(NULL,PAGE_SIZE,0,123,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK);
	test_assertTrue(reg != NULL);
	test_assertSize(reg->getByteCount(),PAGE_SIZE);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertInt(reg->grow(10),0);
	for(i = 1; i < 10; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 11);
	test_assertInt(reg->grow(-5),0);
	for(i = 1; i < 5; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 6);
	test_assertInt(reg->grow(-3),0);
	for(i = 1; i < 2; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 3);
	test_assertInt(reg->grow(-3),0);
	test_assertSize(reg->getByteCount(),0);
	reg->destroy();
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_4(void) {
	Region *reg,*clone;
	test_caseStart("Testing Region::clone()");

	checkMemoryBefore(false);
	reg = Region::create((sFile*)0x1234,PAGE_SIZE,0,123,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK | RF_GROWS_DOWN);
	test_assertTrue(reg != NULL);
	clone = reg->clone((const void*)0x1234);
	test_assertTrue(clone != NULL);
	test_assertPtr(clone->getFile(),(void*)0x1234);
	test_assertULInt(clone->getFlags(),RF_GROWABLE | RF_STACK | RF_GROWS_DOWN);
	test_assertSize(clone->getByteCount(),PAGE_SIZE);
	test_assertSize(reg->refCount(),0);
	test_assertSize(clone->refCount(),1);
	test_assertULInt(clone->getPageFlags(0),PF_DEMANDLOAD);
	reg->destroy();
	clone->destroy();
	checkMemoryAfter(false);

	test_caseSucceeded();
}
