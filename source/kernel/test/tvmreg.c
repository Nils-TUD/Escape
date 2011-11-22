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
#include <sys/task/thread.h>
#include <sys/mem/vmreg.h>
#include <sys/util.h>
#include <esc/test.h>
#include "testutils.h"

#define REG_COUNT	10

static void test_vmreg(void);
static void test_vmreg_inOrder(void);
static void test_vmreg_revOrder(void);
static void test_vmreg_randOrder(void);
static void test_vmreg_addAndRem(uintptr_t *addrs,const char *msg);

/* our test-module */
sTestModule tModVMReg = {
	"VM-Regions",
	&test_vmreg
};

static void test_vmreg(void) {
	test_vmreg_inOrder();
	test_vmreg_revOrder();
	test_vmreg_randOrder();
}

static void test_vmreg_inOrder(void) {
	size_t i;
	uintptr_t addrs[REG_COUNT];
	for(i = 0; i < REG_COUNT; i++)
		addrs[i] = i * 0x1000;
	test_vmreg_addAndRem(addrs,"Add and remove regions with increasing addresses");
}

static void test_vmreg_revOrder(void) {
	size_t i;
	uintptr_t addrs[REG_COUNT];
	for(i = 0; i < REG_COUNT; i++)
		addrs[i] = (REG_COUNT - i) * 0x1000;
	test_vmreg_addAndRem(addrs,"Add and remove regions with decreasing addresses");
}

static void test_vmreg_randOrder(void) {
	size_t i;
	uintptr_t addrs[REG_COUNT];
	for(i = 0; i < REG_COUNT; i++)
		addrs[i] = i * 0x1000;
	util_srand(0x12345);
	for(i = 0; i < 10000; i++) {
		size_t j = util_rand() % REG_COUNT;
		size_t k = util_rand() % REG_COUNT;
		uintptr_t t = addrs[j];
		addrs[j] = addrs[k];
		addrs[k] = t;
	}
	test_vmreg_addAndRem(addrs,"Add and remove regions with addresses in rand order");
}

static void test_vmreg_addAndRem(uintptr_t *addrs,const char *msg) {
	size_t i,j;
	sVMRegTree tree;
	sVMRegion *reg,*regs[REG_COUNT];

	test_caseStart(msg);
	checkMemoryBefore(false);
	vmreg_addTree(0,&tree);

	/* create */
	for(i = 0; i < REG_COUNT; i++) {
		sRegion *r = reg_create(NULL,0,0x1000,0x1000,0,0);
		regs[i] = vmreg_add(&tree,r,addrs[i]);
	}

	/* find all */
	for(i = 0; i < REG_COUNT; i++) {
		reg = vmreg_getByAddr(&tree,addrs[i]);
		test_assertPtr(reg,regs[i]);
		reg = vmreg_getByReg(&tree,regs[i]->reg);
		test_assertPtr(reg,regs[i]);
	}

	/* remove */
	for(i = 0; i < REG_COUNT; i++) {
		sRegion *r = regs[i]->reg;
		reg_destroy(regs[i]->reg);
		vmreg_remove(&tree,regs[i]);
		reg = vmreg_getByAddr(&tree,addrs[i]);
		test_assertPtr(reg,NULL);
		reg = vmreg_getByReg(&tree,r);
		test_assertPtr(reg,NULL);

		for(j = i + 1; j < REG_COUNT; j++) {
			reg = vmreg_getByAddr(&tree,addrs[j]);
			test_assertPtr(reg,regs[j]);
			reg = vmreg_getByReg(&tree,regs[j]->reg);
			test_assertPtr(reg,regs[j]);
		}
	}

	vmreg_remTree(&tree);
	checkMemoryAfter(false);
	test_caseSucceeded();
}
