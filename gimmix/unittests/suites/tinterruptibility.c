/**
 * $Id$
 */

#include "common.h"
#include "core/register.h"
#include "core/cache.h"
#include "core/tc.h"
#include "core/mmu.h"
#include "core/cpu.h"
#include "core/instr/compare.h"
#include "exception.h"
#include "tinterruptibility.h"

#define STACK_ADDR	0x1000000000000

static void test_regset(void);
static void test_regpush(void);
static void test_regpop(void);
static void test_regsave0(void);
static void test_regunsave0(void);
static void test_cswap(void);

const sTestCase tIntrpt[] = {
	{"Register set",test_regset},
	{"Register push",test_regpush},
	{"Register pop",test_regpop},
	{"Register save 0",test_regsave0},
	{"Register unsave 0",test_regunsave0},
	{"CSWAP",test_cswap},
	{NULL,0},
};

static void test_regset(void) {
	reg_setSpecial(rG,255);
	reg_setSpecial(rS,STACK_ADDR);
	reg_setSpecial(rO,STACK_ADDR);

	// first try, which does not change the state at all
	{
		reg_set(254,0x1234);
		reg_push(254);
		test_assertOcta(reg_get(0),0);
		test_assertOcta(reg_getSpecial(rL),0);
		test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
		test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7F8);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_get(0),0);
			test_assertOcta(reg_getSpecial(rL),0);
			test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
			test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7F8);
		}
		else {
			ex_push(&env);
			reg_set(0,0x5678);
			test_assertFalse(true);
		}
		ex_pop();
		reg_pop(0);
	}

	// second try, which sets a few registers first
	{
		reg_setSpecial(rL,0);
		reg_set(252,0x1234);
		reg_push(252);
		test_assertOcta(reg_get(0),0);
		test_assertOcta(reg_get(1),0);
		test_assertOcta(reg_get(2),0);
		test_assertOcta(reg_getSpecial(rL),0);
		test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
		test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7E8);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_get(0),0);
			test_assertOcta(reg_get(1),0);
			test_assertOcta(reg_get(2),0);
			test_assertOcta(reg_getSpecial(rL),2);	// has changed
			test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
			test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7E8);
		}
		else {
			ex_push(&env);
			reg_set(2,0x5678);
			test_assertFalse(true);
		}
		ex_pop();
		reg_pop(0);
	}
}

static void test_regpush(void) {
	reg_setSpecial(rS,STACK_ADDR);
	reg_setSpecial(rO,STACK_ADDR);

	// arrange things so that a push(255) needs to write a register to memory first
	{
		reg_set(254,0x1234);
		reg_push(254);
		test_assertOcta(reg_getSpecial(rL),0);
		test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
		test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7F8);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rL),0);
			test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
			test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7F8);
		}
		else {
			ex_push(&env);
			reg_push(255);
			test_assertFalse(true);
		}
		ex_pop();
		reg_pop(0);
	}

	// arrange things so that a push(0) sets $0, which needs to write a register to memory first
	{
		reg_set(254,0x1234);
		reg_push(254);
		test_assertOcta(reg_getSpecial(rL),0);
		test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
		test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7F8);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rL),0);
			test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
			test_assertOcta(reg_getSpecial(rO),STACK_ADDR + 0x7F8);
		}
		else {
			ex_push(&env);
			reg_push(0);
			test_assertFalse(true);
		}
		ex_pop();
		reg_pop(0);
	}
}

static void test_regpop(void) {
	reg_setSpecial(rL,0);

	// arrange things so that a pop(0) needs to read a register from memory first
	{
		reg_setSpecial(rS,STACK_ADDR);
		reg_setSpecial(rO,STACK_ADDR);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rL),0);
			test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
			test_assertOcta(reg_getSpecial(rO),STACK_ADDR);
		}
		else {
			ex_push(&env);
			reg_pop(0);
			test_assertFalse(true);
		}
		ex_pop();
	}

	// arrange things so that when doing a pop(1), the first loads succeed, but not all
	{
		reg_setSpecial(rS,0x200000000 - 16);
		reg_setSpecial(rO,0x200000000 - 16);
		// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (rwx)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000007,0);
		// PTE 2 for 0x200000000 .. 0x2FFFFFFFF (rwx)
		cache_write(CACHE_DATA,0x400000010,0x0000000500000007,0);

		reg_set(254,0x1234);
		reg_push(254);
		reg_set(253,0x1234);
		test_assertOcta(reg_getSpecial(rL),254);
		test_assertOcta(reg_getSpecial(rS),(0x200000000 - 16) + 0x7F0);
		test_assertOcta(reg_getSpecial(rO),(0x200000000 - 16) + 0x7F8);

		// make it unreadable (0x100000000 .. 0x1FFFFFFFF)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000000,0);
		tc_removeAll(TC_DATA);

		// try a pop(1)
		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			// the last two are left
			test_assertOcta(reg_getSpecial(rL),2);
			// rS has walked back, but stopped at 0x200000000 because the page isn't readable
			test_assertOcta(reg_getSpecial(rS),0x200000000);
			test_assertOcta(reg_getSpecial(rO),(0x200000000 - 16) + 0x7F8);
		}
		else {
			ex_push(&env);
			reg_pop(1);
			test_assertFalse(true);
		}
		ex_pop();

		// call a subroutine with the state left by the unfinished pop
		reg_push(255);
		reg_pop(0);
		test_assertOcta(reg_getSpecial(rL),2);
		test_assertOcta(reg_getSpecial(rS),0x200000008);
		test_assertOcta(reg_getSpecial(rO),(0x200000000 - 16) + 0x7F8);

		// now, make it writable again and retry the pop (0x100000000 .. 0x1FFFFFFFF)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000007,0);
		tc_removeAll(TC_DATA);

		// repeat pop, which should succeed
		reg_pop(1);
		test_assertOcta(reg_getSpecial(rL),255);
		test_assertOcta(reg_getSpecial(rS),0x200000000 - 16);
		test_assertOcta(reg_getSpecial(rO),0x200000000 - 16);

		// undo mapping
		cache_write(CACHE_DATA,0x400000008,0,0);
		cache_write(CACHE_DATA,0x400000010,0,0);
		tc_removeAll(TC_DATA);
	}

	// arrange things so that when doing a pop(1), only the first load succeed
	{
		reg_setSpecial(rL,0);
		reg_setSpecial(rS,0x200000000 - 0x7F0);
		reg_setSpecial(rO,0x200000000 - 0x7F0);
		// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (rwx)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000007,0);
		// PTE 2 for 0x200000000 .. 0x2FFFFFFFF (rwx)
		cache_write(CACHE_DATA,0x400000010,0x0000000500000007,0);

		reg_push(0);
		reg_set(253,0x1234);
		reg_push(253);
		reg_set(254,0x1234);
		test_assertOcta(reg_getSpecial(rL),255);
		test_assertOcta(reg_getSpecial(rS),0x200000008);
		test_assertOcta(reg_getSpecial(rO),0x200000008);

		// make it unreadable (0x100000000 .. 0x1FFFFFFFF)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000000,0);
		tc_removeAll(TC_DATA);

		// try a pop(1)
		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rL),254);
			// rS has walked back, but stopped at 0x200000000 because the page isn't readable
			test_assertOcta(reg_getSpecial(rS),0x200000000);
			test_assertOcta(reg_getSpecial(rO),0x200000008);
		}
		else {
			ex_push(&env);
			reg_pop(1);
			test_assertFalse(true);
		}
		ex_pop();

		// call a subroutine with the state left by the unfinished pop
		reg_push(255);
		reg_pop(0);
		test_assertOcta(reg_getSpecial(rL),254);
		// note that rS has been increased, because the value loaded by the previous pop has been
		// saved again by the push and will be loaded again by the next pop.
		test_assertOcta(reg_getSpecial(rS),0x200000008);
		test_assertOcta(reg_getSpecial(rO),0x200000008);

		// now, make it writable again and retry the pop (0x100000000 .. 0x1FFFFFFFF)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000007,0);
		tc_removeAll(TC_DATA);

		// repeat pop, which should succeed
		reg_pop(1);
		test_assertOcta(reg_getSpecial(rL),254);
		test_assertOcta(reg_getSpecial(rS),0x200000000 - 0x7E8);
		test_assertOcta(reg_getSpecial(rO),0x200000000 - 0x7E8);
		// restore the original state
		reg_pop(0);
		test_assertOcta(reg_getSpecial(rL),0);
		test_assertOcta(reg_getSpecial(rS),0x200000000 - 0x7F0);
		test_assertOcta(reg_getSpecial(rO),0x200000000 - 0x7F0);

		// undo mapping
		cache_write(CACHE_DATA,0x400000008,0,0);
		cache_write(CACHE_DATA,0x400000010,0,0);
		tc_removeAll(TC_DATA);
	}

	// arrange things so that when doing a pop(1), only the first two loads succeed
	{
		reg_setSpecial(rL,0);
		reg_setSpecial(rS,0x200000000 - 0x7E8);
		reg_setSpecial(rO,0x200000000 - 0x7E8);
		// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (rwx)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000007,0);
		// PTE 2 for 0x200000000 .. 0x2FFFFFFFF (rwx)
		cache_write(CACHE_DATA,0x400000010,0x0000000500000007,0);

		reg_push(0);
		reg_set(253,0x1234);
		reg_push(253);
		reg_set(254,0x1234);
		test_assertOcta(reg_getSpecial(rL),255);
		test_assertOcta(reg_getSpecial(rS),0x200000010);
		test_assertOcta(reg_getSpecial(rO),0x200000010);

		// make it unreadable (0x100000000 .. 0x1FFFFFFFF)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000000,0);
		tc_removeAll(TC_DATA);

		// try a pop(1)
		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			// two locals less
			test_assertOcta(reg_getSpecial(rL),253);
			// rS has walked back, but stopped at 0x200000000 because the page isn't readable
			test_assertOcta(reg_getSpecial(rS),0x200000000);
			test_assertOcta(reg_getSpecial(rO),0x200000010);
		}
		else {
			ex_push(&env);
			reg_pop(1);
			test_assertFalse(true);
		}
		ex_pop();

		// call a subroutine with the state left by the unfinished pop
		reg_push(255);
		reg_pop(0);
		test_assertOcta(reg_getSpecial(rL),253);
		// as in the previous test: rS has been increased
		test_assertOcta(reg_getSpecial(rS),0x200000008);
		test_assertOcta(reg_getSpecial(rO),0x200000010);

		// now, make it writable again and retry the pop (0x100000000 .. 0x1FFFFFFFF)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000007,0);
		tc_removeAll(TC_DATA);

		// repeat pop, which should succeed
		reg_pop(1);
		test_assertOcta(reg_getSpecial(rL),254);
		test_assertOcta(reg_getSpecial(rS),0x200000000 - 0x7E0);
		test_assertOcta(reg_getSpecial(rO),0x200000000 - 0x7E0);
		// restore the original state
		reg_pop(0);
		test_assertOcta(reg_getSpecial(rL),0);
		test_assertOcta(reg_getSpecial(rS),0x200000000 - 0x7E8);
		test_assertOcta(reg_getSpecial(rO),0x200000000 - 0x7E8);

		// undo mapping
		cache_write(CACHE_DATA,0x400000008,0,0);
		cache_write(CACHE_DATA,0x400000010,0,0);
		tc_removeAll(TC_DATA);
	}
}

static void test_regsave0(void) {
	// arrange things so that a save fails
	{
		reg_setSpecial(rL,0);
		reg_setSpecial(rS,STACK_ADDR);
		reg_setSpecial(rO,STACK_ADDR);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rL),0);
			test_assertOcta(reg_getSpecial(rS),STACK_ADDR);
			test_assertOcta(reg_getSpecial(rO),STACK_ADDR);
		}
		else {
			ex_push(&env);
			reg_save(255,false);
			test_assertFalse(true);
		}
		ex_pop();
	}

	// arrange things so that when doing a save, the first stores succeed, but not all
	{
		reg_setSpecial(rS,0x100000000 - 48);
		reg_setSpecial(rO,0x100000000 - 48);
		// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (---)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000000,0);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rL),0);
			test_assertOcta(reg_getSpecial(rS),0x100000000 - 48);
			test_assertOcta(reg_getSpecial(rO),0x100000000 - 48);
		}
		else {
			ex_push(&env);
			reg_save(255,false);
			test_assertFalse(true);
		}
		ex_pop();
	}

	// arrange things so that when doing a save, the first stores succeed, but not all
	// and push a few registers down first
	{
		reg_setSpecial(rS,0x100000000 - 258 * sizeof(octa));
		reg_setSpecial(rO,0x100000000 - 258 * sizeof(octa));
		// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (---)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000000,0);

		reg_set(254,0x1234);
		reg_push(254);
		reg_set(10,0x5678);
		test_assertOcta(reg_getSpecial(rL),11);
		test_assertOcta(reg_getSpecial(rS),0xfffff848);
		test_assertOcta(reg_getSpecial(rO),0xffffffe8);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rL),11);
			test_assertOcta(reg_getSpecial(rS),0xfffff848);
			test_assertOcta(reg_getSpecial(rO),0xffffffe8);
		}
		else {
			ex_push(&env);
			reg_save(255,false);
			test_assertFalse(true);
		}
		ex_pop();
	}

	// arrange things so that the save works, but one write more would fail
	{
		// 13 special, rL, 60 locals, 56 globals; 200 regs in the caller
		reg_setSpecial(rS,0x100000000 - (13 + 1 + 60 + 56 + 200 + 1) * sizeof(octa));
		reg_setSpecial(rO,0x100000000 - (13 + 1 + 60 + 56 + 200 + 1) * sizeof(octa));
		// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (---)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000000,0);

		reg_setSpecial(rG,200);
		reg_set(199,0x1234);
		reg_push(199);
		reg_set(60,0x5678);
		test_assertOcta(reg_getSpecial(rL),61);
		// 6 have already been saved because of $60 = 0x5678
		test_assertOcta(reg_getSpecial(rS),0x100000000 - (13 + 1 + 60 + 56 + 200 + 1 - 6) * sizeof(octa));
		// 200 have been pushed down
		test_assertOcta(reg_getSpecial(rO),0x100000000 - (13 + 1 + 60 + 56 + 1) * sizeof(octa));
		reg_save(255,false);
		test_assertOcta(reg_getSpecial(rL),0);
		// this proves that one write more would have failed
		test_assertOcta(reg_getSpecial(rS),0x100000000);
		test_assertOcta(reg_getSpecial(rO),0x100000000);
	}

	// undo mapping
	cache_write(CACHE_DATA,0x400000008,0,0);
	cache_write(CACHE_DATA,0x400000010,0,0);
	tc_removeAll(TC_DATA);
}

static void test_regunsave0(void) {
	static octa specials[SPECIAL_NUM];
	static octa locals[LREG_NUM];
	static octa globals[GREG_NUM];

	// set some state
	reg_setSpecial(rG,100);
	reg_setSpecial(rS,0x1000);
	reg_setSpecial(rO,0x1000);
	for(size_t i = 100; i < GREG_NUM; i++)
		reg_setGlobal(i,i);
	for(size_t i = 0; i < 10; i++)
		reg_set(i,i);

	// backup
	for(size_t i = 0; i < LREG_NUM; i++)
		locals[i] = reg_get(i);
	for(size_t i = 100; i < GREG_NUM; i++)
		globals[i] = reg_getGlobal(i);
	for(size_t i = 0; i < SPECIAL_NUM; i++)
		specials[i] = reg_getSpecial(i);

	// arrange things so that an unsave fails at the beginning
	{
		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			for(size_t i = 0; i < LREG_NUM; i++)
				test_assertOcta(reg_get(i),locals[i]);
			for(size_t i = 100; i < GREG_NUM; i++)
				test_assertOcta(reg_getGlobal(i),globals[i]);
			for(size_t i = 0; i < SPECIAL_NUM; i++)
				test_assertOcta(reg_getSpecial(i),specials[i]);
		}
		else {
			ex_push(&env);
			reg_unsave(STACK_ADDR,false);
			test_assertFalse(true);
		}
		ex_pop();
	}

	// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (---)
	cache_write(CACHE_DATA,0x400000008,0x0000000400000000,0);
	// PTE 2 for 0x200000000 .. 0x2FFFFFFFF (rwx)
	cache_write(CACHE_DATA,0x400000010,0x0000000500000007,0);

	// arrange things so that an unsave fails when reading rL
	{
		// set rG|rA
		mmu_writeOcta(0x200000020,0xFE00000000000000,MEM_SIDE_EFFECTS);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			for(size_t i = 0; i < LREG_NUM; i++)
				test_assertOcta(reg_get(i),locals[i]);
			for(size_t i = 100; i < GREG_NUM; i++)
				test_assertOcta(reg_getGlobal(i),globals[i]);
			for(size_t i = 0; i < SPECIAL_NUM; i++)
				test_assertOcta(reg_getSpecial(i),specials[i]);
		}
		else {
			ex_push(&env);
			reg_unsave(0x200000020,false);
			test_assertFalse(true);
		}
		ex_pop();
	}

	// arrange things so that an unsave fails when testing the range
	{
		// set rG|rA
		mmu_writeOcta(0x200000080,0xFE00000000000000,MEM_SIDE_EFFECTS);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			for(size_t i = 0; i < LREG_NUM; i++)
				test_assertOcta(reg_get(i),locals[i]);
			for(size_t i = 100; i < GREG_NUM; i++)
				test_assertOcta(reg_getGlobal(i),globals[i]);
			for(size_t i = 0; i < SPECIAL_NUM; i++)
				test_assertOcta(reg_getSpecial(i),specials[i]);
		}
		else {
			ex_push(&env);
			reg_unsave(0x200000080,false);
			test_assertFalse(true);
		}
		ex_pop();
	}

	// arrange things so that it works, but one value more would fail
	{
		// we have 13 specials, rL, rL locals and rG globals
		octa off = (13 + 1 + 10 + (256 - 254) - 1) * sizeof(octa);
		// set rG|rA
		mmu_writeOcta(0x200000000 + off,0xFE00000000000000,MEM_SIDE_EFFECTS);
		// rL is saved 13+rG positions further
		mmu_writeOcta(0x200000000 + off - (13 + (256 - 254)) * sizeof(octa),10,MEM_SIDE_EFFECTS);

		reg_unsave(0x200000000 + off,false);
		test_assertTrue(true);
		test_assertOcta(reg_getSpecial(rG),254);
		test_assertOcta(reg_getSpecial(rL),10);
		// this proves that one read more would have failed
		test_assertOcta(reg_getSpecial(rS),0x200000000);
		test_assertOcta(reg_getSpecial(rO),0x200000000);
	}

	// undo mapping
	cache_write(CACHE_DATA,0x400000008,0,0);
	cache_write(CACHE_DATA,0x400000010,0,0);
	tc_removeAll(TC_DATA);
}

static void test_cswap(void) {
	sInstrArgs iargs;
	reg_setSpecial(rL,0);
	reg_setSpecial(rG,255);

	// arrange things so that reading the memory-location fails
	{
		// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (-wx)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000003,0);
		tc_removeAll(TC_DATA);

		reg_setSpecial(rP,0x1234567890ABCDEF);
		reg_set(10,0x1234);
		mmu_writeOcta(0x100000000,0,MEM_SIDE_EFFECTS);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rP),0x1234567890ABCDEF);
			test_assertOcta(reg_get(10),0x1234);
			test_assertOcta(cache_read(CACHE_DATA,0x400000000,MEM_SIDE_EFFECTS),0);
		}
		else {
			ex_push(&env);
			iargs.x = 10;
			iargs.y = 0x100000000;
			iargs.z = 0;
			cpu_instr_cswap(&iargs);
			test_assertFalse(true);
		}
		ex_pop();
	}

	// arrange things so that writing the memory-location fails
	{
		reg_setSpecial(rP,0x1234567890ABCDEF);
		reg_set(10,0x1234);
		mmu_writeOcta(0x100000000,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);

		// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (r-x)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000005,0);
		tc_removeAll(TC_DATA);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rP),0x1234567890ABCDEF);
			test_assertOcta(reg_get(10),0x1234);
			test_assertOcta(mmu_readOcta(0x100000000,MEM_SIDE_EFFECTS),0x1234567890ABCDEF);
		}
		else {
			ex_push(&env);
			iargs.x = 10;
			iargs.y = 0x100000000;
			iargs.z = 0;
			cpu_instr_cswap(&iargs);
			test_assertFalse(true);
		}
		ex_pop();
	}

	// arrange things so that setting register X fails
	{
		// PTE 1 for 0x100000000 .. 0x1FFFFFFFF (rwx)
		cache_write(CACHE_DATA,0x400000008,0x0000000400000007,0);
		tc_removeAll(TC_DATA);

		reg_setSpecial(rP,0x1234567890ABCDEF);
		mmu_writeOcta(0x100000000,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);

		// push some registers down
		reg_setSpecial(rO,STACK_ADDR);
		reg_setSpecial(rS,STACK_ADDR);
		reg_set(254,0x1234);
		reg_push(254);

		jmp_buf env;
		int ex = setjmp(env);
		if(ex != EX_NONE) {
			test_assertOcta(reg_getSpecial(rP),0x1234567890ABCDEF);
			test_assertOcta(reg_getSpecial(rL),0);
			test_assertOcta(mmu_readOcta(0x100000000,MEM_SIDE_EFFECTS),0x1234567890ABCDEF);
		}
		else {
			ex_push(&env);
			iargs.x = 10;
			iargs.y = 0x100000000;
			iargs.z = 0;
			cpu_instr_cswap(&iargs);
			test_assertFalse(true);
		}
		ex_pop();
	}
}
