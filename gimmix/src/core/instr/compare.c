/**
 * $Id: compare.c 192 2011-04-29 12:14:36Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/compare.h"
#include "core/mmu.h"
#include "core/register.h"

static void zeroOrSetReg(const sInstrArgs *iargs,bool set);

void cpu_instr_cmp(const sInstrArgs *iargs) {
	octa res;
	octa op1 = iargs->y;
	octa op2 = iargs->z;
	// op1 < 0 and op2 >= 0?
	if((op1 & MSB(64)) > (op2 & MSB(64)))
		res = -1;
	// op1 >= 0 and op2 < 0?
	else if((op1 & MSB(64)) < (op2 & MSB(64)))
		res = 1;
	// now we can assume that both are >= 0 or both are < 0
	// therefore we can compare with unsigned arithmetic
	else if(op1 < op2)
		res = -1;
	else if(op1 > op2)
		res = 1;
	else
		res = 0;
	reg_set(iargs->x,res);
}

void cpu_instr_cmpu(const sInstrArgs *iargs) {
	octa res;
	if(iargs->y < iargs->z)
		res = -1;
	else if(iargs->y > iargs->z)
		res = 1;
	else
		res = 0;
	reg_set(iargs->x,res);
}

void cpu_instr_csn(const sInstrArgs *iargs) {
	if(iargs->y & MSB(64))
		reg_set(iargs->x,iargs->z);
}

void cpu_instr_csz(const sInstrArgs *iargs) {
	if(iargs->y == 0)
		reg_set(iargs->x,iargs->z);
}

void cpu_instr_csp(const sInstrArgs *iargs) {
	if(!(iargs->y & MSB(64)) && iargs->y != 0)
		reg_set(iargs->x,iargs->z);
}

void cpu_instr_csod(const sInstrArgs *iargs) {
	if(iargs->y & 1)
		reg_set(iargs->x,iargs->z);
}

void cpu_instr_csnn(const sInstrArgs *iargs) {
	if(!(iargs->y & MSB(64)))
		reg_set(iargs->x,iargs->z);
}

void cpu_instr_csnz(const sInstrArgs *iargs) {
	if(iargs->y != 0)
		reg_set(iargs->x,iargs->z);
}

void cpu_instr_csnp(const sInstrArgs *iargs) {
	if((iargs->y & MSB(64)) || iargs->y == 0)
		reg_set(iargs->x,iargs->z);
}

void cpu_instr_csev(const sInstrArgs *iargs) {
	if(!(iargs->y & 1))
		reg_set(iargs->x,iargs->z);
}

void cpu_instr_zsn(const sInstrArgs *iargs) {
	zeroOrSetReg(iargs,iargs->y & MSB(64));
}

void cpu_instr_zsz(const sInstrArgs *iargs) {
	zeroOrSetReg(iargs,iargs->y == 0);
}

void cpu_instr_zsp(const sInstrArgs *iargs) {
	zeroOrSetReg(iargs,!(iargs->y & MSB(64)) && iargs->y != 0);
}

void cpu_instr_zsod(const sInstrArgs *iargs) {
	zeroOrSetReg(iargs,iargs->y & 1);
}

void cpu_instr_zsnn(const sInstrArgs *iargs) {
	zeroOrSetReg(iargs,!(iargs->y & MSB(64)));
}

void cpu_instr_zsnz(const sInstrArgs *iargs) {
	zeroOrSetReg(iargs,iargs->y != 0);
}

void cpu_instr_zsnp(const sInstrArgs *iargs) {
	zeroOrSetReg(iargs,(iargs->y & MSB(64)) || iargs->y == 0);
}

void cpu_instr_zsev(const sInstrArgs *iargs) {
	zeroOrSetReg(iargs,!(iargs->y & 1));
}

void cpu_instr_cswap(const sInstrArgs *iargs) {
	octa addr = iargs->y + iargs->z;
	octa mem = mmu_readOcta(addr,MEM_SIDE_EFFECTS);
	if(mem == reg_getSpecial(rP)) {
		// we have the problem here that both reg_set and mmu_writeOcta might throw an exception
		// therefore we use the following trick...
		octa val = reg_get(iargs->x);
		// first, set X to an arbitrary value. if it throws, we haven't change anything yet
		reg_set(iargs->x,0);
		// if not, we're here and reset the register
		reg_set(iargs->x,val);
		// if mmu_writeOcta throws, we haven't changed anything yet
		mmu_writeOcta(addr,val,MEM_SIDE_EFFECTS);
		// if not, we're here and set X again. but this can't throw again since it would have
		// already done that before
		reg_set(iargs->x,1);
	}
	else {
		// reg_set has to be done first, because it might throw an exception
		reg_set(iargs->x,0);
		reg_setSpecial(rP,mem);
	}
}

static void zeroOrSetReg(const sInstrArgs *iargs,bool set) {
	octa val = set ? iargs->z : 0;
	reg_set(iargs->x,val);
}
