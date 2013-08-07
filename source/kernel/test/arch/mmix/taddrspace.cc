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
#include <sys/arch/mmix/mem/addrspace.h>
#include <sys/task/proc.h>
#include <sys/mem/paging.h>
#include <esc/test.h>

#define ADDR_SPACE_COUNT		1024

/* forward declarations */
static void test_addrspace();
static void test_basics();
static void test_dupUsage();

/* our test-module */
sTestModule tModAddrSpace = {
	"Address space",
	&test_addrspace
};
static AddressSpace *spaces[ADDR_SPACE_COUNT * 3];

static void test_addrspace() {
	Proc *p = Proc::getByPid(Proc::getRunning());
	/* a trick to ensure that no address-spaces are in use yet: temporary free the first one and
	 * allocate it again when we're finished */
	AddressSpace::free(p->getPageDir()->addrSpace);
	test_basics();
	test_dupUsage();
	/* do it twice to ensure that the cleanup is correct */
	test_dupUsage();
	p->getPageDir()->addrSpace = AddressSpace::alloc();
	p->getPageDir()->rv &= ~0x3F;
	p->getPageDir()->rv |= (p->getPageDir()->addrSpace->getNo() << 3);
}

static void test_basics() {
	AddressSpace *as1,*as2;
	test_caseStart("Testing basics");

	as1 = AddressSpace::alloc();
	as2 = AddressSpace::alloc();
	test_assertTrue(as1 != as2);
	test_assertTrue(as1->getNo() != as2->getNo());
	test_assertUInt(as1->getRefCount(),1);
	test_assertUInt(as2->getRefCount(),1);

	AddressSpace::free(as2);
	AddressSpace::free(as1);

	test_caseSucceeded();
}

static void test_dupUsage() {
	test_caseStart("Testing duplicate usage");

	/* allocate 0 .. 1023 */
	for(size_t i = 0; i < ADDR_SPACE_COUNT; i++) {
		spaces[i] = AddressSpace::alloc();
	}
	for(size_t i = 0; i < ADDR_SPACE_COUNT; i++) {
		test_assertUInt(spaces[i]->getRefCount(),1);
	}

	/* allocate 1024 .. 2047 */
	for(size_t i = 0; i < ADDR_SPACE_COUNT; i++) {
		spaces[ADDR_SPACE_COUNT + i] = AddressSpace::alloc();
	}
	for(size_t i = 0; i < ADDR_SPACE_COUNT; i++) {
		test_assertUInt(spaces[ADDR_SPACE_COUNT + i]->getRefCount(),2);
	}

	/* allocate 2048 .. 3071 */
	for(size_t i = 0; i < ADDR_SPACE_COUNT; i++) {
		spaces[ADDR_SPACE_COUNT * 2 + i] = AddressSpace::alloc();
	}
	for(size_t i = 0; i < ADDR_SPACE_COUNT; i++) {
		test_assertUInt(spaces[ADDR_SPACE_COUNT * 2 + i]->getRefCount(),3);
	}

	/* free 3071 .. 2048 */
	for(size_t i = 0; i < ADDR_SPACE_COUNT; i++) {
		AddressSpace::free(spaces[ADDR_SPACE_COUNT * 2 + i]);
		test_assertUInt(spaces[ADDR_SPACE_COUNT * 2 + i]->getRefCount(),2);
	}
	/* free 2047 .. 1024 */
	for(size_t i = 0; i < ADDR_SPACE_COUNT; i++) {
		AddressSpace::free(spaces[ADDR_SPACE_COUNT + i]);
		test_assertUInt(spaces[ADDR_SPACE_COUNT + i]->getRefCount(),1);
	}
	/* free 2047 .. 0 */
	for(size_t i = 0; i < ADDR_SPACE_COUNT; i++) {
		AddressSpace::free(spaces[i]);
		test_assertUInt(spaces[i]->getRefCount(),0);
	}

	test_caseSucceeded();
}
