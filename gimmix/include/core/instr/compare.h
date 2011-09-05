/**
 * $Id: compare.h 192 2011-04-29 12:14:36Z nasmussen $
 */

#ifndef COMPARE_H_
#define COMPARE_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_cmp(const sInstrArgs *iargs);
void cpu_instr_cmpu(const sInstrArgs *iargs);

void cpu_instr_csn(const sInstrArgs *iargs);
void cpu_instr_csz(const sInstrArgs *iargs);
void cpu_instr_csp(const sInstrArgs *iargs);
void cpu_instr_csod(const sInstrArgs *iargs);
void cpu_instr_csnn(const sInstrArgs *iargs);
void cpu_instr_csnz(const sInstrArgs *iargs);
void cpu_instr_csnp(const sInstrArgs *iargs);
void cpu_instr_csev(const sInstrArgs *iargs);

void cpu_instr_zsn(const sInstrArgs *iargs);
void cpu_instr_zsz(const sInstrArgs *iargs);
void cpu_instr_zsp(const sInstrArgs *iargs);
void cpu_instr_zsod(const sInstrArgs *iargs);
void cpu_instr_zsnn(const sInstrArgs *iargs);
void cpu_instr_zsnz(const sInstrArgs *iargs);
void cpu_instr_zsnp(const sInstrArgs *iargs);
void cpu_instr_zsev(const sInstrArgs *iargs);

void cpu_instr_cswap(const sInstrArgs *iargs);

#endif /* COMPARE_H_ */
