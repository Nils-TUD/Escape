/**
 * $Id: load.c 171 2011-04-14 13:18:30Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/load.h"
#include "core/register.h"
#include "core/mmu.h"

static void cpu_instr_loadByte(byte dst,octa addr,bool sign);
static void cpu_instr_loadWyde(byte dst,octa addr,bool sign);
static void cpu_instr_loadTetra(byte dst,octa addr,bool sign);
static void cpu_instr_loadOcta(byte dst,octa addr);
static void cpu_instr_loadHighTetra(byte dst,octa addr);

void cpu_instr_ldb(const sInstrArgs *iargs) {
	cpu_instr_loadByte(iargs->x,iargs->y,true);
}

void cpu_instr_ldbu(const sInstrArgs *iargs) {
	cpu_instr_loadByte(iargs->x,iargs->y,false);
}

void cpu_instr_ldw(const sInstrArgs *iargs) {
	cpu_instr_loadWyde(iargs->x,iargs->y,true);
}

void cpu_instr_ldwu(const sInstrArgs *iargs) {
	cpu_instr_loadWyde(iargs->x,iargs->y,false);
}

void cpu_instr_ldt(const sInstrArgs *iargs) {
	cpu_instr_loadTetra(iargs->x,iargs->y,true);
}

void cpu_instr_ldtu(const sInstrArgs *iargs) {
	cpu_instr_loadTetra(iargs->x,iargs->y,false);
}

void cpu_instr_ldo(const sInstrArgs *iargs) {
	cpu_instr_loadOcta(iargs->x,iargs->y);
}

void cpu_instr_ldou(const sInstrArgs *iargs) {
	cpu_instr_loadOcta(iargs->x,iargs->y);
}

void cpu_instr_ldht(const sInstrArgs *iargs) {
	cpu_instr_loadHighTetra(iargs->x,iargs->y);
}

static void cpu_instr_loadByte(byte dst,octa addr,bool sign) {
	byte b = mmu_readByte(addr,MEM_SIDE_EFFECTS);
	reg_set(dst,sign ? SEXT(b,8) : ZEXT(b,8));
}

static void cpu_instr_loadWyde(byte dst,octa addr,bool sign) {
	wyde w = mmu_readWyde(addr,MEM_SIDE_EFFECTS);
	reg_set(dst,sign ? SEXT(w,16) : ZEXT(w,16));
}

static void cpu_instr_loadTetra(byte dst,octa addr,bool sign) {
	tetra t = mmu_readTetra(addr,MEM_SIDE_EFFECTS);
	reg_set(dst,sign ? SEXT(t,32) : ZEXT(t,32));
}

static void cpu_instr_loadOcta(byte dst,octa addr) {
	octa o = mmu_readOcta(addr,MEM_SIDE_EFFECTS);
	reg_set(dst,o);
}

static void cpu_instr_loadHighTetra(byte dst,octa addr) {
	tetra t = mmu_readTetra(addr,MEM_SIDE_EFFECTS);
	reg_set(dst,(octa)t << 32);
}
