/**
 * $Id: load.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef LOAD_H_
#define LOAD_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_ldb(const sInstrArgs *iargs);
void cpu_instr_ldbu(const sInstrArgs *iargs);

void cpu_instr_ldw(const sInstrArgs *iargs);
void cpu_instr_ldwu(const sInstrArgs *iargs);

void cpu_instr_ldt(const sInstrArgs *iargs);
void cpu_instr_ldtu(const sInstrArgs *iargs);

void cpu_instr_ldo(const sInstrArgs *iargs);
void cpu_instr_ldou(const sInstrArgs *iargs);

void cpu_instr_ldht(const sInstrArgs *iargs);

#endif /* LOAD_H_ */
