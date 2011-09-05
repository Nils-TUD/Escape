/**
 * $Id: wyde.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef WYDE_H_
#define WYDE_H_

#include "common.h"
#include "core/decoder.h"

void cpu_instr_seth(const sInstrArgs *iargs);
void cpu_instr_setmh(const sInstrArgs *iargs);
void cpu_instr_setml(const sInstrArgs *iargs);
void cpu_instr_setl(const sInstrArgs *iargs);

void cpu_instr_inch(const sInstrArgs *iargs);
void cpu_instr_incmh(const sInstrArgs *iargs);
void cpu_instr_incml(const sInstrArgs *iargs);
void cpu_instr_incl(const sInstrArgs *iargs);

void cpu_instr_orh(const sInstrArgs *iargs);
void cpu_instr_ormh(const sInstrArgs *iargs);
void cpu_instr_orml(const sInstrArgs *iargs);
void cpu_instr_orl(const sInstrArgs *iargs);

void cpu_instr_andnh(const sInstrArgs *iargs);
void cpu_instr_andnmh(const sInstrArgs *iargs);
void cpu_instr_andnml(const sInstrArgs *iargs);
void cpu_instr_andnl(const sInstrArgs *iargs);

#endif /* WYDE_H_ */
