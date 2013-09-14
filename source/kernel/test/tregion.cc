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
#include <sys/mem/pagedir.h>
#include <sys/video.h>
#include <sys/cppsupport.h>
#include <esc/test.h>
#include "tregion.h"
#include "testutils.h"

/* forward declarations */
static void test_region();
static void test_1();
static void test_2();
static void test_3();

/* our test-module */
sTestModule tModRegion = {
	"VMM Regions",
	&test_region
};

static void test_region() {
	test_1();
	test_2();
	test_3();
}

static void test_1() {
	Region *reg;
	test_caseStart("Testing Region::create() & Region::destroy()");

	checkMemoryBefore(false);
	reg = CREATE(Region,nullptr,124,124,123,PF_DEMANDLOAD,RF_GROWABLE);
	test_assertTrue(reg != NULL);
	test_assertPtr(reg->getFile(),NULL);
	test_assertULInt(reg->getFlags(),RF_GROWABLE);
	test_assertSize(reg->getByteCount(),124);
	test_assertSize(reg->refCount(),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	delete reg;
	checkMemoryAfter(false);

	checkMemoryBefore(false);
	reg = CREATE(Region,(OpenFile*)0x1234,PAGE_SIZE + 1,PAGE_SIZE + 1,124,0,RF_STACK | RF_GROWS_DOWN);
	test_assertTrue(reg != NULL);
	test_assertPtr(reg->getFile(),(void*)0x1234);
	test_assertULInt(reg->getFlags(),RF_STACK | RF_GROWS_DOWN);
	test_assertSize(reg->getByteCount(),PAGE_SIZE + 1);
	test_assertSize(reg->refCount(),0);
	test_assertULInt(reg->getPageFlags(0),0);
	test_assertULInt(reg->getPageFlags(1),0);
	delete reg;
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_2() {
	Region *reg;
	test_caseStart("Testing Region::grow()");

	checkMemoryBefore(false);
	reg = CREATE(Region,nullptr,PAGE_SIZE,PAGE_SIZE,123,PF_DEMANDLOAD,RF_GROWABLE);
	test_assertTrue(reg != NULL);
	test_assertSize(reg->getByteCount(),PAGE_SIZE);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertInt(reg->grow(10),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	for(size_t i = 1; i < 11; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 11);
	test_assertInt(reg->grow(-5),0);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 6);
	test_assertInt(reg->grow(-3),0);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 3);
	test_assertInt(reg->grow(-3),0);
	test_assertSize(reg->getByteCount(),0);
	delete reg;
	checkMemoryAfter(false);

	checkMemoryBefore(false);
	reg = CREATE(Region,nullptr,PAGE_SIZE,0,123,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK | RF_GROWS_DOWN);
	test_assertTrue(reg != NULL);
	test_assertSize(reg->getByteCount(),PAGE_SIZE);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertInt(reg->grow(10),0);
	for(size_t i = 0; i < 10; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(10),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 11);
	test_assertInt(reg->grow(-5),0);
	for(size_t i = 0; i < 5; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(5),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 6);
	test_assertInt(reg->grow(-3),0);
	for(size_t i = 0; i < 2; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(2),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 3);
	test_assertInt(reg->grow(-3),0);
	test_assertSize(reg->getByteCount(),0);
	delete reg;
	checkMemoryAfter(false);

	checkMemoryBefore(false);
	reg = CREATE(Region,nullptr,PAGE_SIZE,0,123,PF_DEMANDLOAD,RF_GROWABLE | RF_STACK);
	test_assertTrue(reg != NULL);
	test_assertSize(reg->getByteCount(),PAGE_SIZE);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertInt(reg->grow(10),0);
	for(size_t i = 1; i < 10; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 11);
	test_assertInt(reg->grow(-5),0);
	for(size_t i = 1; i < 5; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 6);
	test_assertInt(reg->grow(-3),0);
	for(size_t i = 1; i < 2; i++)
		test_assertULInt(reg->getPageFlags(i),0);
	test_assertULInt(reg->getPageFlags(0),PF_DEMANDLOAD);
	test_assertSize(reg->getByteCount(),PAGE_SIZE * 3);
	test_assertInt(reg->grow(-3),0);
	test_assertSize(reg->getByteCount(),0);
	delete reg;
	checkMemoryAfter(false);

	test_caseSucceeded();
}

static void test_3() {
	Region *reg,*reg2;
	VirtMem vm;
	test_caseStart("Testing Region::clone()");

	checkMemoryBefore(false);
	reg = CREATE(Region,(OpenFile*)0x1234,PAGE_SIZE,0,123,PF_DEMANDLOAD,
			RF_GROWABLE | RF_STACK | RF_GROWS_DOWN);
	test_assertTrue(reg != NULL);
	reg2 = CLONE(Region,*reg,&vm);
	test_assertTrue(reg2 != NULL);
	test_assertPtr(reg2->getFile(),(void*)0x1234);
	test_assertULInt(reg2->getFlags(),RF_GROWABLE | RF_STACK | RF_GROWS_DOWN);
	test_assertSize(reg2->getByteCount(),PAGE_SIZE);
	test_assertSize(reg->refCount(),0);
	test_assertSize(reg2->refCount(),1);
	test_assertULInt(reg2->getPageFlags(0),PF_DEMANDLOAD);
	delete reg;
	delete reg2;
	checkMemoryAfter(false);

	test_caseSucceeded();
}
