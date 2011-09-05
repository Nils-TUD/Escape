/**
 * $Id: morebit.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef MOREBIT_H_
#define MOREBIT_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_bdif(const sInstrArgs *iargs);
void cpu_instr_wdif(const sInstrArgs *iargs);
void cpu_instr_tdif(const sInstrArgs *iargs);
void cpu_instr_odif(const sInstrArgs *iargs);

void cpu_instr_sadd(const sInstrArgs *iargs);

void cpu_instr_mor(const sInstrArgs *iargs);
void cpu_instr_mxor(const sInstrArgs *iargs);

#endif /* MOREBIT_H_ */
