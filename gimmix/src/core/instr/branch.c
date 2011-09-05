/**
 * $Id: branch.c 171 2011-04-14 13:18:30Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/branch.h"
#include "core/register.h"
#include "exception.h"

static void jumpTo(octa addr);

void cpu_instr_jmp(const sInstrArgs *iargs) {
	jumpTo(iargs->z);
}

void cpu_instr_go(const sInstrArgs *iargs) {
	// note that this behaviour differs from MMIX. we won't change our state if the requested pc
	// is not legal. MMIX sets the pc first and checks it afterwards. That also means that MMIX
	// stores the new pc in rWW
	octa npc = iargs->y + iargs->z;
	if(!cpu_isPCOk(npc))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_PC);

	// now set the register (which may throw an exception) and set the new pc
	reg_set(iargs->x,cpu_getPC() + sizeof(tetra));
	// substract sizeof(tetra) because it will be increased afterwards
	cpu_setPC(npc - sizeof(tetra));
}

void cpu_instr_bn(const sInstrArgs *iargs) {
	if(iargs->x & MSB(64))
		jumpTo(iargs->z);
}

void cpu_instr_bz(const sInstrArgs *iargs) {
	if(iargs->x == 0)
		jumpTo(iargs->z);
}

void cpu_instr_bp(const sInstrArgs *iargs) {
	if(!(iargs->x & MSB(64)) && iargs->x != 0)
		jumpTo(iargs->z);
}

void cpu_instr_bod(const sInstrArgs *iargs) {
	if(iargs->x & 1)
		jumpTo(iargs->z);
}

void cpu_instr_bnn(const sInstrArgs *iargs) {
	if(!(iargs->x & MSB(64)))
		jumpTo(iargs->z);
}

void cpu_instr_bnz(const sInstrArgs *iargs) {
	if(iargs->x != 0)
		jumpTo(iargs->z);
}

void cpu_instr_bnp(const sInstrArgs *iargs) {
	if((iargs->x & MSB(64)) || iargs->x == 0)
		jumpTo(iargs->z);
}

void cpu_instr_bev(const sInstrArgs *iargs) {
	if(!(iargs->x & 1))
		jumpTo(iargs->z);
}

static void jumpTo(octa addr) {
	if(!cpu_isPCOk(addr))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_PC);
	// substract sizeof(tetra) because it will be increased
	cpu_setPC(addr - sizeof(tetra));
}
