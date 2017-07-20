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

#include <mem/region.h>
#include <mem/vmtree.h>
#include <sys/test.h>
#include <task/thread.h>
#include <common.h>
#include <cppsupport.h>
#include <util.h>

#include "testutils.h"

#define TEST_REG_COUNT	10

static void test_vmreg();
static void test_vmreg_inOrder();
static void test_vmreg_revOrder();
static void test_vmreg_randOrder();
static void test_vmreg_addAndRem(uintptr_t *addrs,const char *msg);

/* our test-module */
sTestModule tModVMReg = {
	"VM-Regions",
	&test_vmreg
};

static void test_vmreg() {
	test_vmreg_inOrder();
	test_vmreg_revOrder();
	test_vmreg_randOrder();
}

static void test_vmreg_inOrder() {
	uintptr_t addrs[TEST_REG_COUNT];
	for(size_t i = 0; i < TEST_REG_COUNT; i++)
		addrs[i] = i * PAGE_SIZE;
	test_vmreg_addAndRem(addrs,"Add and remove regions with increasing addresses");
}

static void test_vmreg_revOrder() {
	uintptr_t addrs[TEST_REG_COUNT];
	for(size_t i = 0; i < TEST_REG_COUNT; i++)
		addrs[i] = (TEST_REG_COUNT - i) * PAGE_SIZE;
	test_vmreg_addAndRem(addrs,"Add and remove regions with decreasing addresses");
}

static void test_vmreg_randOrder() {
	uintptr_t addrs[TEST_REG_COUNT];
	for(size_t i = 0; i < TEST_REG_COUNT; i++)
		addrs[i] = i * PAGE_SIZE;
	Util::srand(0x12345);
	for(size_t i = 0; i < 10000; i++) {
		size_t j = Util::rand() % TEST_REG_COUNT;
		size_t k = Util::rand() % TEST_REG_COUNT;
		uintptr_t t = addrs[j];
		addrs[j] = addrs[k];
		addrs[k] = t;
	}
	test_vmreg_addAndRem(addrs,"Add and remove regions with addresses in rand order");
}

static void test_vmreg_addAndRem(uintptr_t *addrs,const char *msg) {
	VMTree tree(NULL);
	VMRegion *reg,*regs[TEST_REG_COUNT];

	test_caseStart(msg);
	checkMemoryBefore(false);
	VMTree::addTree(&tree);

	/* create */
	for(size_t i = 0; i < TEST_REG_COUNT; i++) {
		Region *r = createObj<Region>(nullptr,PAGE_SIZE,0,0,0,0);
		regs[i] = tree.add(r,addrs[i]);
	}

	/* find all */
	for(size_t i = 0; i < TEST_REG_COUNT; i++) {
		reg = tree.getByAddr(addrs[i]);
		test_assertPtr(reg,regs[i]);
		reg = tree.getByReg(regs[i]->reg);
		test_assertPtr(reg,regs[i]);
	}

	/* remove */
	for(size_t i = 0; i < TEST_REG_COUNT; i++) {
		Region *r = regs[i]->reg;
		tree.remove(regs[i]);
		delete r;
		reg = tree.getByAddr(addrs[i]);
		test_assertPtr(reg,NULL);
		reg = tree.getByReg(r);
		test_assertPtr(reg,NULL);

		for(size_t j = i + 1; j < TEST_REG_COUNT; j++) {
			reg = tree.getByAddr(addrs[j]);
			test_assertPtr(reg,regs[j]);
			reg = tree.getByReg(regs[j]->reg);
			test_assertPtr(reg,regs[j]);
		}
	}

	VMTree::remTree(&tree);
	checkMemoryAfter(false);
	test_caseSucceeded();
}
