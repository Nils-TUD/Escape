/**
 * $Id: misc.h 192 2011-04-29 12:14:36Z nasmussen $
 */

#ifndef MISC_H_
#define MISC_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_swym(const sInstrArgs *iargs);
void cpu_instr_ldvts(const sInstrArgs *iargs);
void cpu_instr_geta(const sInstrArgs *iargs);

void cpu_instr_push(const sInstrArgs *iargs);
void cpu_instr_pop(const sInstrArgs *iargs);

void cpu_instr_save(const sInstrArgs *iargs);
void cpu_instr_unsave(const sInstrArgs *iargs);

void cpu_instr_get(const sInstrArgs *iargs);
void cpu_instr_put(const sInstrArgs *iargs);

#endif /* MISC_H_ */
