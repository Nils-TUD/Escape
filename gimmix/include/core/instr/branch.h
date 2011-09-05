/**
 * $Id: branch.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef BRANCH_H_
#define BRANCH_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_jmp(const sInstrArgs *iargs);
void cpu_instr_go(const sInstrArgs *iargs);

void cpu_instr_bn(const sInstrArgs *iargs);
void cpu_instr_bz(const sInstrArgs *iargs);
void cpu_instr_bp(const sInstrArgs *iargs);
void cpu_instr_bod(const sInstrArgs *iargs);
void cpu_instr_bnn(const sInstrArgs *iargs);
void cpu_instr_bnz(const sInstrArgs *iargs);
void cpu_instr_bnp(const sInstrArgs *iargs);
void cpu_instr_bev(const sInstrArgs *iargs);

#endif /* BRANCH_H_ */
