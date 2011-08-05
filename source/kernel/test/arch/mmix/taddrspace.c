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
static void test_addrspace(void);
static void test_basics(void);
static void test_dupUsage(void);

/* our test-module */
sTestModule tModAddrSpace = {
	"Address space",
	&test_addrspace
};
static sAddressSpace *spaces[ADDR_SPACE_COUNT * 3];

static void test_addrspace(void) {
	sProc *p = proc_getByPid(proc_getRunning());
	/* a trick to ensure that no address-spaces are in use yet: temporary free the first one and
	 * allocate it again when we're finished */
	aspace_free(p->pagedir.addrSpace);
	test_basics();
	test_dupUsage();
	/* do it twice to ensure that the cleanup is correct */
	test_dupUsage();
	p->pagedir.addrSpace = aspace_alloc();
	p->pagedir.rv &= ~0x3F;
	p->pagedir.rv |= (p->pagedir.addrSpace->no << 3);
}

static void test_basics(void) {
	sAddressSpace *as1,*as2;
	test_caseStart("Testing basics");

	as1 = aspace_alloc();
	as2 = aspace_alloc();
	test_assertTrue(as1 != as2);
	test_assertTrue(as1->no != as2->no);
	test_assertUInt(as1->refCount,1);
	test_assertUInt(as2->refCount,1);

	aspace_free(as2);
	aspace_free(as1);

	test_caseSucceeded();
}

static void test_dupUsage(void) {
	size_t i;
	test_caseStart("Testing duplicate usage");

	/* allocate 0 .. 1023 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		spaces[i] = aspace_alloc();
	}
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		test_assertUInt(spaces[i]->refCount,1);
	}

	/* allocate 1024 .. 2047 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		spaces[ADDR_SPACE_COUNT + i] = aspace_alloc();
	}
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		test_assertUInt(spaces[ADDR_SPACE_COUNT + i]->refCount,2);
	}

	/* allocate 2048 .. 3071 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		spaces[ADDR_SPACE_COUNT * 2 + i] = aspace_alloc();
	}
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		test_assertUInt(spaces[ADDR_SPACE_COUNT * 2 + i]->refCount,3);
	}

	/* free 3071 .. 2048 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		aspace_free(spaces[ADDR_SPACE_COUNT * 2 + i]);
		test_assertUInt(spaces[ADDR_SPACE_COUNT * 2 + i]->refCount,2);
	}
	/* free 2047 .. 1024 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		aspace_free(spaces[ADDR_SPACE_COUNT + i]);
		test_assertUInt(spaces[ADDR_SPACE_COUNT + i]->refCount,1);
	}
	/* free 2047 .. 0 */
	for(i = 0; i < ADDR_SPACE_COUNT; i++) {
		aspace_free(spaces[i]);
		test_assertUInt(spaces[i]->refCount,0);
	}

	test_caseSucceeded();
}
