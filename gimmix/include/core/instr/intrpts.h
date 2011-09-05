/**
 * $Id: intrpts.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef INTRPTS_H_
#define INTRPTS_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_trap(const sInstrArgs *iargs);
void cpu_instr_trip(const sInstrArgs *iargs);

void cpu_instr_resume(const sInstrArgs *iargs);

#endif /* INTRPTS_H_ */
