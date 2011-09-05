/**
 * $Id: store.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef STORE_H_
#define STORE_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_stb(const sInstrArgs *iargs);
void cpu_instr_stbu(const sInstrArgs *iargs);

void cpu_instr_stw(const sInstrArgs *iargs);
void cpu_instr_stwu(const sInstrArgs *iargs);

void cpu_instr_stt(const sInstrArgs *iargs);
void cpu_instr_sttu(const sInstrArgs *iargs);

void cpu_instr_sto(const sInstrArgs *iargs);
void cpu_instr_stou(const sInstrArgs *iargs);

void cpu_instr_stht(const sInstrArgs *iargs);

void cpu_instr_stco(const sInstrArgs *iargs);

#endif /* STORE_H_ */
