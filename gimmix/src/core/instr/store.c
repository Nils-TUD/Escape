/**
 * $Id: store.c 171 2011-04-14 13:18:30Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/store.h"
#include "core/register.h"
#include "core/mmu.h"
#include "exception.h"

static void cpu_instr_storeByte(octa val,octa addr,bool sign);
static void cpu_instr_storeWyde(octa val,octa addr,bool sign);
static void cpu_instr_storeTetra(octa val,octa addr,bool sign);

void cpu_instr_stb(const sInstrArgs *iargs) {
	cpu_instr_storeByte(iargs->x,iargs->y,true);
}

void cpu_instr_stbu(const sInstrArgs *iargs) {
	cpu_instr_storeByte(iargs->x,iargs->y,false);
}

void cpu_instr_stw(const sInstrArgs *iargs) {
	cpu_instr_storeWyde(iargs->x,iargs->y,true);
}

void cpu_instr_stwu(const sInstrArgs *iargs) {
	cpu_instr_storeWyde(iargs->x,iargs->y,false);
}

void cpu_instr_stt(const sInstrArgs *iargs) {
	cpu_instr_storeTetra(iargs->x,iargs->y,true);
}

void cpu_instr_sttu(const sInstrArgs *iargs) {
	cpu_instr_storeTetra(iargs->x,iargs->y,false);
}

void cpu_instr_sto(const sInstrArgs *iargs) {
	mmu_writeOcta(iargs->y,iargs->x,MEM_SIDE_EFFECTS);
}

void cpu_instr_stou(const sInstrArgs *iargs) {
	mmu_writeOcta(iargs->y,iargs->x,MEM_SIDE_EFFECTS);
}

void cpu_instr_stht(const sInstrArgs *iargs) {
	mmu_writeTetra(iargs->y,iargs->x >> 32,MEM_SIDE_EFFECTS);
}

void cpu_instr_stco(const sInstrArgs *iargs) {
	mmu_writeOcta(iargs->y,iargs->x,MEM_SIDE_EFFECTS);
}

static void cpu_instr_storeByte(octa val,octa addr,bool sign) {
	// we can store the following values in a byte:
	// #FFFFFFFFFFFFFF80 = -128 .. #FFFFFFFFFFFFFFFF = -1
	// #0000000000000000 = 0    .. #000000000000007F = 127
	// at first, negative values. there are 3 ranges:
	// 1. #FFFFFFFFFFFFFFFF .. #FFFFFFFFFFFFFF80
	//    these values are storable in a byte. and (val << 56) >> 56 doesn't change anything
	// 2. #FFFFFFFFFFFFFF7F .. #FFFFFFFFFFFFFF00
	//    these values are not storable in a byte. because bit #80 is not set, the arithmetical
	//    right shift will produce zeros and therefore the result is different
	// 3. #FFFFFFFFFFFFFEFF .. #8000000000000000
	//    these values are not storable in a byte. regardless of whether bit #80 is set or not we
	//    can't produce the same value again. because either we get #FFFFFFFFFFFFFFXX or
	//    #00000000000000XX.
	// now, the positive values. we have 2 ranges:
	// 1. #0000000000000000 .. #000000000000007F
	//    these values are storable in a byte. and (val << 56) >> 56 doesn't change anything
	// 2. #0000000000000080 .. #7FFFFFFFFFFFFFFF
	//    these values are not storable in a byte. if bit #80 is set, the MSB will be set
	//    after the shifts -> differs. if not, the value has set a bit in the upper 56 bits, which
	//    will be zero'd by the shifts.
	if(sign && ((socta)(val << 56) >> 56) != (socta)val)
		cpu_setArithEx(TRIP_INTOVER);
	mmu_writeByte(addr,val & 0xFF,MEM_SIDE_EFFECTS);
}
static void cpu_instr_storeWyde(octa val,octa addr,bool sign) {
	// same as above
	if(sign && ((socta)(val << 48) >> 48) != (socta)val)
		cpu_setArithEx(TRIP_INTOVER);
	mmu_writeWyde(addr,val & 0xFFFF,MEM_SIDE_EFFECTS);
}
static void cpu_instr_storeTetra(octa val,octa addr,bool sign) {
	// same as above
	if(sign && ((socta)(val << 32) >> 32) != (socta)val)
		cpu_setArithEx(TRIP_INTOVER);
	mmu_writeTetra(addr,val & 0xFFFFFFFF,MEM_SIDE_EFFECTS);
}
