/**
 * $Id: bit.c 171 2011-04-14 13:18:30Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/bit.h"
#include "core/arith/int.h"
#include "core/register.h"
#include "exception.h"

void cpu_instr_and(const sInstrArgs *iargs) {
	octa res = iargs->y & iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_or(const sInstrArgs *iargs) {
	octa res = iargs->y | iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_xor(const sInstrArgs *iargs) {
	octa res = iargs->y ^ iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_andn(const sInstrArgs *iargs) {
	octa res = iargs->y & ~iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_orn(const sInstrArgs *iargs) {
	octa res = iargs->y | ~iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_nand(const sInstrArgs *iargs) {
	octa res = ~(iargs->y & iargs->z);
	reg_set(iargs->x,res);
}

void cpu_instr_nor(const sInstrArgs *iargs) {
	octa res = ~(iargs->y | iargs->z);
	reg_set(iargs->x,res);
}

void cpu_instr_nxor(const sInstrArgs *iargs) {
	octa res = ~(iargs->y ^ iargs->z);
	reg_set(iargs->x,res);
}

void cpu_instr_mux(const sInstrArgs *iargs) {
	octa res = (iargs->y & reg_getSpecial(rM)) | (iargs->z & ~reg_getSpecial(rM));
	reg_set(iargs->x,res);
}

void cpu_instr_sl(const sInstrArgs *iargs) {
	octa res = int_shiftLeft(iargs->y,iargs->z);
	octa a = (socta)res >> iargs->z;
	if(a != iargs->y)
		cpu_setArithEx(TRIP_INTOVER);
	reg_set(iargs->x,res);
}

void cpu_instr_slu(const sInstrArgs *iargs) {
	octa res = int_shiftLeft(iargs->y,iargs->z);
	reg_set(iargs->x,res);
}

void cpu_instr_sr(const sInstrArgs *iargs) {
	octa res = int_shiftRightArith(iargs->y,iargs->z);
	reg_set(iargs->x,res);
}

void cpu_instr_sru(const sInstrArgs *iargs) {
	octa res = int_shiftRightLog(iargs->y,iargs->z);
	reg_set(iargs->x,res);
}
