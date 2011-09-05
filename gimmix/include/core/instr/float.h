/**
 * $Id: float.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef FLOAT_H_
#define FLOAT_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_fmul(const sInstrArgs *iargs);
void cpu_instr_fdiv(const sInstrArgs *iargs);

void cpu_instr_fadd(const sInstrArgs *iargs);
void cpu_instr_fsub(const sInstrArgs *iargs);

void cpu_instr_fcmp(const sInstrArgs *iargs);
void cpu_instr_feql(const sInstrArgs *iargs);
void cpu_instr_fun(const sInstrArgs *iargs);

void cpu_instr_fcmpe(const sInstrArgs *iargs);
void cpu_instr_feqle(const sInstrArgs *iargs);
void cpu_instr_fune(const sInstrArgs *iargs);

void cpu_instr_fint(const sInstrArgs *iargs);
void cpu_instr_fix(const sInstrArgs *iargs);
void cpu_instr_fixu(const sInstrArgs *iargs);

void cpu_instr_flot(const sInstrArgs *iargs);
void cpu_instr_flotu(const sInstrArgs *iargs);
void cpu_instr_sflot(const sInstrArgs *iargs);
void cpu_instr_sflotu(const sInstrArgs *iargs);

void cpu_instr_fsqrt(const sInstrArgs *iargs);
void cpu_instr_frem(const sInstrArgs *iargs);

void cpu_instr_ldsf(const sInstrArgs *iargs);
void cpu_instr_stsf(const sInstrArgs *iargs);

#endif /* FLOAT_H_ */
