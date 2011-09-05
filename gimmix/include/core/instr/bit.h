/**
 * $Id: bit.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef BIT_H_
#define BIT_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_and(const sInstrArgs *iargs);
void cpu_instr_or(const sInstrArgs *iargs);
void cpu_instr_xor(const sInstrArgs *iargs);
void cpu_instr_andn(const sInstrArgs *iargs);
void cpu_instr_orn(const sInstrArgs *iargs);
void cpu_instr_nand(const sInstrArgs *iargs);
void cpu_instr_nor(const sInstrArgs *iargs);
void cpu_instr_nxor(const sInstrArgs *iargs);

void cpu_instr_mux(const sInstrArgs *iargs);

void cpu_instr_sl(const sInstrArgs *iargs);
void cpu_instr_slu(const sInstrArgs *iargs);
void cpu_instr_sr(const sInstrArgs *iargs);
void cpu_instr_sru(const sInstrArgs *iargs);

#endif /* BIT_H_ */
