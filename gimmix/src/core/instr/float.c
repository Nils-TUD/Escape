/**
 * $Id: float.c 171 2011-04-14 13:18:30Z nasmussen $
 */

#include "common.h"
#include "core/decoder.h"
#include "core/instr/float.h"
#include "core/arith/float.h"
#include "core/cpu.h"
#include "core/register.h"
#include "core/mmu.h"
#include "exception.h"

static octa cpu_instr_doFix(octa z,octa y,bool unSigned);
static octa cpu_instr_doFlot(octa z,octa y,bool unSigned,bool shortPrec);

void cpu_instr_fmul(const sInstrArgs *iargs) {
	unsigned exceptions = 0;
	octa res = fl_mult(iargs->y,iargs->z,&exceptions);
	if(exceptions)
		cpu_setArithEx(exceptions);
	reg_set(iargs->x,res);
}

void cpu_instr_fdiv(const sInstrArgs *iargs) {
	unsigned exceptions = 0;
	octa res = fl_divide(iargs->y,iargs->z,&exceptions);
	if(exceptions)
		cpu_setArithEx(exceptions);
	reg_set(iargs->x,res);
}

void cpu_instr_fadd(const sInstrArgs *iargs) {
	unsigned exceptions = 0;
	octa res = fl_add(iargs->y,iargs->z,&exceptions);
	if(exceptions)
		cpu_setArithEx(exceptions);
	reg_set(iargs->x,res);
}

void cpu_instr_fsub(const sInstrArgs *iargs) {
	unsigned exceptions = 0;
	octa res = fl_sub(iargs->y,iargs->z,&exceptions);
	if(exceptions)
		cpu_setArithEx(exceptions);
	reg_set(iargs->x,res);
}

void cpu_instr_fcmp(const sInstrArgs *iargs) {
	octa res = fl_compare(iargs->y,iargs->z);
	if(res == CMP_UNORDERED) {
		cpu_setArithEx(TRIP_FLOATINVOP);
		res = CMP_EQUAL;
	}
	reg_set(iargs->x,res);
}

void cpu_instr_feql(const sInstrArgs *iargs) {
	octa res = fl_compare(iargs->y,iargs->z);
	reg_set(iargs->x,res == CMP_EQUAL ? 1 : 0);
}

void cpu_instr_fun(const sInstrArgs *iargs) {
	octa res = fl_compare(iargs->y,iargs->z);
	reg_set(iargs->x,res == CMP_UNORDERED ? 1 : 0);
}

void cpu_instr_fcmpe(const sInstrArgs *iargs) {
	octa res = fl_compareWithEps(iargs->y,iargs->z,reg_getSpecial(rE),true);
	if(res == ECMP_INVALID) {
		cpu_setArithEx(TRIP_FLOATINVOP);
		res = CMP_EQUAL;
	}
	else if(res == ECMP_NOT_EQUAL)
		res = fl_compare(iargs->y,iargs->z);
	else
		res = CMP_EQUAL;
	reg_set(iargs->x,res);
}

void cpu_instr_feqle(const sInstrArgs *iargs) {
	octa res = fl_compareWithEps(iargs->y,iargs->z,reg_getSpecial(rE),false);
	if(res == ECMP_INVALID) {
		cpu_setArithEx(TRIP_FLOATINVOP);
		res = 0;
	}
	reg_set(iargs->x,res);
}

void cpu_instr_fune(const sInstrArgs *iargs) {
	octa res = fl_compareWithEps(iargs->y,iargs->z,reg_getSpecial(rE),true);
	reg_set(iargs->x,res == ECMP_INVALID ? 1 : 0);
}

void cpu_instr_fint(const sInstrArgs *iargs) {
	unsigned exceptions = 0;
	if(iargs->y > 4)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	octa res = fl_interize(iargs->z,iargs->y,&exceptions);
	if(exceptions)
		cpu_setArithEx(exceptions);
	reg_set(iargs->x,res);
}

void cpu_instr_fix(const sInstrArgs *iargs) {
	reg_set(iargs->x,cpu_instr_doFix(iargs->z,iargs->y,false));
}

void cpu_instr_fixu(const sInstrArgs *iargs) {
	reg_set(iargs->x,cpu_instr_doFix(iargs->z,iargs->y,true));
}

void cpu_instr_flot(const sInstrArgs *iargs) {
	reg_set(iargs->x,cpu_instr_doFlot(iargs->z,iargs->y,false,false));
}

void cpu_instr_flotu(const sInstrArgs *iargs) {
	reg_set(iargs->x,cpu_instr_doFlot(iargs->z,iargs->y,true,false));
}

void cpu_instr_sflot(const sInstrArgs *iargs) {
	reg_set(iargs->x,cpu_instr_doFlot(iargs->z,iargs->y,false,true));
}

void cpu_instr_sflotu(const sInstrArgs *iargs) {
	reg_set(iargs->x,cpu_instr_doFlot(iargs->z,iargs->y,true,true));
}

void cpu_instr_fsqrt(const sInstrArgs *iargs) {
	unsigned exceptions = 0;
	if(iargs->y > 4)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	octa res = fl_sqrt(iargs->z,iargs->y,&exceptions);
	if(exceptions)
		cpu_setArithEx(exceptions);
	reg_set(iargs->x,res);
}

void cpu_instr_frem(const sInstrArgs *iargs) {
	unsigned exceptions = 0;
	// 2500 makes sure that the remainder can be calculated in one step
	octa res = fl_remstep(iargs->y,iargs->z,2500,&exceptions);
	if(exceptions)
		cpu_setArithEx(exceptions);
	reg_set(iargs->x,res);
}

void cpu_instr_ldsf(const sInstrArgs *iargs) {
	unsigned exceptions = 0;
	tetra x = mmu_readTetra(iargs->y,MEM_SIDE_EFFECTS);
	octa ox = fl_shortToFloat(x,&exceptions);
	// we don't report exceptions here!
	reg_set(iargs->x,ox);
}

void cpu_instr_stsf(const sInstrArgs *iargs) {
	unsigned exceptions = 0;
	tetra tx = fl_floatToShort(iargs->x,&exceptions);
	if(exceptions)
		cpu_setArithEx(exceptions);
	mmu_writeTetra(iargs->y,tx,MEM_SIDE_EFFECTS);
}

static octa cpu_instr_doFix(octa z,octa y,bool unSigned) {
	unsigned exceptions = 0;
	if(y > 4)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	octa res = fl_fixit(z,y,&exceptions);
	if(unSigned)
		exceptions &= ~TRIP_FLOAT2FIXOVER;
	if(exceptions)
		cpu_setArithEx(exceptions);
	return res;
}

static octa cpu_instr_doFlot(octa z,octa y,bool unSigned,bool shortPrec) {
	unsigned exceptions = 0;
	if(y > 4)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	octa res = fl_floatit(z,y,unSigned,shortPrec,&exceptions);
	if(exceptions)
		cpu_setArithEx(exceptions);
	return res;
}
