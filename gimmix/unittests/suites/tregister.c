/**
 * $Id: tregister.c 130 2011-02-19 13:42:05Z nasmussen $
 */

#include "common.h"
#include "core/register.h"
#include "tregister.h"

#define STACK_ADDR	0x6000000000000000

static void test_dyn(void);
static void test_global(void);
static void test_pushpop(void);
static void test_stack(void);
static void test_saveunsave(void);

const sTestCase tRegister[] = {
	{"Dynamic registers",test_dyn},
	{"Global registers",test_global},
	{"Test push/pop",test_pushpop},
	{"Stack",test_stack},
	{"Save/unsave",test_saveunsave},
	{NULL,0},
};

static void test_dyn(void) {
	test_assertInt(reg_getSpecial(rL),0);

	reg_set(0,0x1234);
	test_assertInt(reg_getSpecial(rL),1);
	test_assertOcta(reg_get(0),0x1234);
	test_assertOcta(reg_get(1),0);

	reg_set(4,0x4567);
	test_assertInt(reg_getSpecial(rL),5);
	test_assertOcta(reg_get(1),0);
	test_assertOcta(reg_get(2),0);
	test_assertOcta(reg_get(3),0);
	test_assertOcta(reg_get(4),0x4567);

	// reset
	reg_setSpecial(rL,0);
}

static void test_global(void) {
	test_assertOcta(reg_getSpecial(rL),0);
	reg_setSpecial(rG,128);

	reg_set(128,0xDEADBEEF);
	test_assertOcta(reg_getGlobal(128),0xDEADBEEF);
	test_assertOcta(reg_getSpecial(rL),0);

	reg_setSpecial(rG,256);
}

static void test_pushpop(void) {
	test_assertOcta(reg_getSpecial(rL),0);

	// 0 arg, 0 return
	reg_push(0);
	test_assertOcta(reg_getSpecial(rL),0);
	reg_pop(0);
	test_assertOcta(reg_getSpecial(rL),0);

	// 1 arg, 0 return
	reg_set(0,0x1234);
	reg_set(2,0x5678);
	reg_push(1);
	test_assertOcta(reg_getSpecial(rL),1);
	reg_pop(0);
	test_assertOcta(reg_getSpecial(rL),1);
	test_assertOcta(reg_get(0),0x1234);

	// 1 arg, 1 return
	reg_set(0,0x1234);
	reg_set(2,0x5678);
	reg_push(1);
	test_assertOcta(reg_getSpecial(rL),1);
	reg_pop(1);
	test_assertOcta(reg_getSpecial(rL),2);
	test_assertOcta(reg_get(0),0x1234);
	test_assertOcta(reg_get(1),0x5678);

	// 0 arg, 1 return
	reg_setSpecial(rL,0);
	reg_push(0);
	test_assertOcta(reg_getSpecial(rL),0);
	reg_set(0,0xDEAD);
	reg_pop(1);
	test_assertOcta(reg_getSpecial(rL),1);
	test_assertOcta(reg_get(0),0xDEAD);

	reg_setSpecial(rL,0);
}

static void test_stack(void) {
	test_assertOcta(reg_getSpecial(rL),0);

	// stores nothing
	reg_set(254,0x1234);
	test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
	test_assertOcta(reg_getSpecial(rO),STACK_ADDR);
	// the push sets one register and increases the offset by L=256 * 8
	reg_push(255);
	test_assertOcta(reg_getSpecial(rS),STACK_ADDR + 0x8);
	test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x800);
	test_assertOcta(reg_getSpecial(rL),0);
	// pop restores the registers and decreases the offset
	reg_pop(0);
	test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
	test_assertOcta(reg_getSpecial(rO),STACK_ADDR);
	test_assertOcta(reg_getSpecial(rL),255);
	test_assertOcta(reg_get(254),0x1234);

	// now push without stack-effects
	reg_set(0,0xDEAD);
	reg_set(1,0xBAAA);
	reg_set(2,0xFAAA);
	reg_set(253,0xBEEF);
	reg_setSpecial(rL,254);
	reg_push(254);
	test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
	test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7F8);
	// each set stores one reg
	reg_set(0,0xFF);
	reg_set(1,0xFF);
	reg_set(2,0xFF);
	test_assertOcta(reg_getSpecial(rS),STACK_ADDR + 0x18);
	test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7F8);
	// restores the regs from stack
	reg_pop(0);
	test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
	test_assertOcta(reg_getSpecial(rO),STACK_ADDR);
	test_assertOcta(reg_getSpecial(rL),254);
	test_assertOcta(reg_get(0),0xDEAD);
	test_assertOcta(reg_get(1),0xBAAA);
	test_assertOcta(reg_get(2),0xFAAA);
	test_assertOcta(reg_get(253),0xBEEF);

	reg_setSpecial(rL,0);
}

static void test_saveunsave(void) {
	// the saved special-regs
	static int sregs[] = {rB,rD,rE,rH,rJ,rM,rR,rP,rW,rX,rY,rZ,rA};
	test_assertOcta(reg_getSpecial(rL),0);

	// set global regs, special regs and local regs
	reg_setSpecial(rG,127);
	for(int i = 128; i < 255; i++)
		reg_set(i,i);
	for(size_t i = 0; i < ARRAY_SIZE(sregs); i++)
		reg_setSpecial(sregs[i],i);
	for(int i = 0; i < 10; i++)
		reg_set(i,i);

	reg_save(127,false);

	// set other values
	for(int i = 128; i < 255; i++)
		reg_set(i,i + 1);
	for(size_t i = 0; i < ARRAY_SIZE(sregs); i++)
		reg_setSpecial(sregs[i],i + 1);
	for(int i = 0; i < 10; i++)
		reg_set(i,i + 1);

	reg_unsave(reg_get(127),false);

	// compare with the original ones
	for(int i = 128; i < 255; i++)
		test_assertOcta(reg_get(i),i);
	for(size_t i = 0; i < ARRAY_SIZE(sregs); i++)
		test_assertOcta(reg_getSpecial(sregs[i]),i);
	for(int i = 0; i < 10; i++)
		test_assertOcta(reg_get(i),i);
}
