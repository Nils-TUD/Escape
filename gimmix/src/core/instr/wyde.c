/**
 * $Id: wyde.c 171 2011-04-14 13:18:30Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/wyde.h"
#include "core/register.h"

void cpu_instr_seth(const sInstrArgs *iargs) {
	reg_set(iargs->x,iargs->z << 48);
}
void cpu_instr_setmh(const sInstrArgs *iargs) {
	reg_set(iargs->x,iargs->z << 32);
}
void cpu_instr_setml(const sInstrArgs *iargs) {
	reg_set(iargs->x,iargs->z << 16);
}
void cpu_instr_setl(const sInstrArgs *iargs) {
	reg_set(iargs->x,iargs->z << 0);
}

void cpu_instr_inch(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val + (iargs->z << 48));
}
void cpu_instr_incmh(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val + (iargs->z << 32));
}
void cpu_instr_incml(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val + (iargs->z << 16));
}
void cpu_instr_incl(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val + (iargs->z << 0));
}

void cpu_instr_orh(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val | (iargs->z << 48));
}
void cpu_instr_ormh(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val | (iargs->z << 32));
}
void cpu_instr_orml(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val | (iargs->z << 16));
}
void cpu_instr_orl(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val | (iargs->z << 0));
}

void cpu_instr_andnh(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val & ~(iargs->z << 48));
}
void cpu_instr_andnmh(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val & ~(iargs->z << 32));
}
void cpu_instr_andnml(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val & ~(iargs->z << 16));
}
void cpu_instr_andnl(const sInstrArgs *iargs) {
	octa val = reg_get(iargs->x);
	reg_set(iargs->x,val & ~(iargs->z << 0));
}
