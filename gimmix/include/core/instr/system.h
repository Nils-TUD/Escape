/**
 * $Id: system.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_ldunc(const sInstrArgs *iargs);
void cpu_instr_stunc(const sInstrArgs *iargs);

void cpu_instr_preld(const sInstrArgs *iargs);
void cpu_instr_prego(const sInstrArgs *iargs);
void cpu_instr_prest(const sInstrArgs *iargs);

void cpu_instr_syncd(const sInstrArgs *iargs);
void cpu_instr_syncid(const sInstrArgs *iargs);

void cpu_instr_sync(const sInstrArgs *iargs);

#endif /* SYSTEM_H_ */
