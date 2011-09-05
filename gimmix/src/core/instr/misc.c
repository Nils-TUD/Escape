/**
 * $Id: misc.c 207 2011-05-14 11:55:06Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/misc.h"
#include "core/register.h"
#include "core/mmu.h"
#include "core/tc.h"
#include "core/bus.h"
#include "exception.h"
#include "mmix/error.h"

static octa lastrQ = 0;

void cpu_instr_swym(const sInstrArgs *iargs) {
	UNUSED(iargs);
	// does nothing
}

void cpu_instr_ldvts(const sInstrArgs *iargs) {
	octa key = iargs->y + iargs->z;
	int prot = key & 0x7;
	key &= ~(octa)0x7;

	int res = 0;
	for(int i = 0; i < 2; i++) {
		sTCEntry *tce = tc_getByKey(i,key);
		if(tce) {
			if(prot) {
				tce->translation &= ~(octa)0x7;
				tce->translation |= prot;
			}
			else
				tc_remove(tce);
			res |= 1 << i;
		}
	}
	reg_set(iargs->x,res);
}

void cpu_instr_geta(const sInstrArgs *iargs) {
	reg_set(iargs->x,iargs->y);
}

void cpu_instr_push(const sInstrArgs *iargs) {
	if(!cpu_isPCOk(iargs->z))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_PC);

	// push may throw an exception
	reg_push(iargs->x);
	// these 2 may NOT throw an exception
	reg_setSpecial(rJ,cpu_getPC() + sizeof(tetra));
	cpu_setPC(iargs->z - sizeof(tetra));
}

void cpu_instr_pop(const sInstrArgs *iargs) {
	if(!cpu_isPCOk(iargs->z))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_PC);

	// pop may throw an exception
	reg_pop(iargs->x);
	// cpu_setPC may NOT
	cpu_setPC(iargs->z - sizeof(tetra));
}

void cpu_instr_save(const sInstrArgs *iargs) {
	if(iargs->x < reg_getSpecial(rG))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	if(iargs->z > 1)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	reg_save(iargs->x,iargs->z == 1);
}

void cpu_instr_unsave(const sInstrArgs *iargs) {
	if(iargs->x > 1)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	reg_unsave(iargs->y,iargs->x == 1);
}

void cpu_instr_get(const sInstrArgs *iargs) {
	if(iargs->z >= SPECIAL_NUM)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	// if we want to get rL, the reg has to be set first (for the case that dst >= rL)
	// the same for rS, if we have to store something on the stack first
	// Note that if the first reg_set does not throw an exception, the second won't, either
	if(iargs->z == rL || iargs->z == rS)
		reg_set(iargs->x,0);
	octa value = reg_getSpecial(iargs->z);
	reg_set(iargs->x,value);
	// save value for rQ (see cpu_instr_put)
	if(iargs->z == rQ)
		lastrQ = value;
}

void cpu_instr_put(const sInstrArgs *iargs) {
	if(iargs->x >= SPECIAL_NUM)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	if(iargs->x >= rC) {
		// rC, rN, rO, rS (not changeable in general)
		if(iargs->x <= rS)
			ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
		// rI, rT, rTT, rK, rQ, rU, rV, rSS (not changeable for user-apps)
		if((iargs->x <= rV || iargs->x == rSS) && !cpu_isPriv())
			ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_INSTR);
	}
	// in rA only the bits 0x3FFFF are valid
	if(iargs->x == rA && iargs->z & ~(octa)0x3FFFF)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	// max(32,rL,GREG_NUM) <= rG < 256
	if(iargs->x == rG && (iargs->z >= 256 || iargs->z < 256 - GREG_NUM ||
					iargs->z < reg_getSpecial(rL) || iargs->z < MIN_GLOBAL))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	// kernel-stack HAS TO be in kernel-space
	if(iargs->x == rSS && !(iargs->z & MSB(64)))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	// when rQ is set, the bits that have been changed from 0 to 1 since the last GET x,rQ can't
	// be unset. therefore we OR them in again.
	octa value = iargs->z;
	if(iargs->x == rQ) {
		octa newBits = reg_getSpecial(rQ) & ~lastrQ;
		value |= newBits;
		// we have to make sure that only valid bits in rQ can be set
		if(value & ~(bus_getIntrptBits() | TRAP_EX_BITS))
			ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	}
	// rL will not be increased by PUT
	if(iargs->x == rL)
		value = MIN(reg_getSpecial(rL),value);
	reg_setSpecial(iargs->x,value);
}
