/**
 * $Id: arith.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef ARITH_H_
#define ARITH_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_mul(const sInstrArgs *iargs);
void cpu_instr_mulu(const sInstrArgs *iargs);

void cpu_instr_div(const sInstrArgs *iargs);
void cpu_instr_divu(const sInstrArgs *iargs);

void cpu_instr_add(const sInstrArgs *iargs);
void cpu_instr_addu(const sInstrArgs *iargs);

void cpu_instr_sub(const sInstrArgs *iargs);
void cpu_instr_subu(const sInstrArgs *iargs);

void cpu_instr_2addu(const sInstrArgs *iargs);
void cpu_instr_4addu(const sInstrArgs *iargs);
void cpu_instr_8addu(const sInstrArgs *iargs);
void cpu_instr_16addu(const sInstrArgs *iargs);

void cpu_instr_neg(const sInstrArgs *iargs);
void cpu_instr_negu(const sInstrArgs *iargs);

#endif /* ARITH_H_ */
