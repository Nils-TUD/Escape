/**
 * $Id: arith.c 171 2011-04-14 13:18:30Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/arith.h"
#include "core/arith/int.h"
#include "core/register.h"
#include "exception.h"
#include "mmix/error.h"

void cpu_instr_mul(const sInstrArgs *iargs) {
	octa aux;
	octa res = int_smult(iargs->y,iargs->z,&aux);
	// its an overflow if and only if aux is unequal to 64 copies of the sign bit in the result
	// i.e. its an overflow if we multiply 2 large positive numbers or 2 large negative numbers
	if(aux != ((res & MSB(64)) ? 0xFFFFFFFFFFFFFFFF : 0))
		cpu_setArithEx(TRIP_INTOVER);
	reg_set(iargs->x,res);
}

void cpu_instr_mulu(const sInstrArgs *iargs) {
	octa aux;
	octa res = int_umult(iargs->y,iargs->z,&aux);
	reg_set(iargs->x,res);
	reg_setSpecial(rH,aux);
}

void cpu_instr_div(const sInstrArgs *iargs) {
	octa rem;
	octa res = int_sdiv(iargs->y,iargs->z,&rem);
	if(iargs->z == 0)
		cpu_setArithEx(TRIP_INTDIVCHK);
	if((iargs->y & MSB(64)) && (iargs->z & MSB(64)) && (res >> 32) == MSB(32))
		cpu_setArithEx(TRIP_INTOVER);
	reg_set(iargs->x,res);
	reg_setSpecial(rR,rem);
}

void cpu_instr_divu(const sInstrArgs *iargs) {
	octa rem;
	octa res = int_udiv(reg_getSpecial(rD),iargs->y,iargs->z,&rem);
	reg_set(iargs->x,res);
	reg_setSpecial(rR,rem);
}

void cpu_instr_add(const sInstrArgs *iargs) {
	octa res = iargs->y + iargs->z;
	// its an overflow, if the MSB is the same for the operands and different in the result
	if(((iargs->y ^ iargs->z) & MSB(64)) == 0 && ((iargs->y ^ res) & MSB(64)) != 0)
		cpu_setArithEx(TRIP_INTOVER);
	reg_set(iargs->x,res);
}

void cpu_instr_addu(const sInstrArgs *iargs) {
	octa res = iargs->y + iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_sub(const sInstrArgs *iargs) {
	octa res = iargs->y - iargs->z;
	// its an overflow, if the MSB is the same for res and z and different for res and y
	// i.e. if one of:
	// - neg = pos - neg
	// - pos = neg - pos
	if(((res ^ iargs->z) & MSB(64)) == 0 && ((res ^ iargs->y) & MSB(64)) != 0)
		cpu_setArithEx(TRIP_INTOVER);
	reg_set(iargs->x,res);
}

void cpu_instr_subu(const sInstrArgs *iargs) {
	octa res = iargs->y - iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_2addu(const sInstrArgs *iargs) {
	octa res = (iargs->y << 1) + iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_4addu(const sInstrArgs *iargs) {
	octa res = (iargs->y << 2) + iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_8addu(const sInstrArgs *iargs) {
	octa res = (iargs->y << 3) + iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_16addu(const sInstrArgs *iargs) {
	octa res = (iargs->y << 4) + iargs->z;
	reg_set(iargs->x,res);
}

void cpu_instr_neg(const sInstrArgs *iargs) {
	octa res = iargs->y - iargs->z;
	// Note: its NOT an overflow, if the result is > 2^63-1 (as described in mmix-doc.pdf)!
	if(((res ^ iargs->z) & MSB(64)) == 0 && ((res ^ iargs->y) & MSB(64)) != 0)
		cpu_setArithEx(TRIP_INTOVER);
	reg_set(iargs->x,res);
}

void cpu_instr_negu(const sInstrArgs *iargs) {
	octa res = iargs->y - iargs->z;
	reg_set(iargs->x,res);
}
