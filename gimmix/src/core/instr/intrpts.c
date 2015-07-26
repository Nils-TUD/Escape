/**
 * $Id: intrpts.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/instr/intrpts.h"
#include "core/register.h"
#include "core/decoder.h"
#include "core/tc.h"
#include "core/mmu.h"
#include "mmix/error.h"
#include "exception.h"
#include "config.h"

#define RESUME_AGAIN	0	// repeat the command in rX as if in location rW - 4
#define RESUME_CONT		1	// same, but substitute rY and rZ for operands
#define RESUME_SET		2	// set r[X] to rZ
#define RESUME_TRANS	3	// install (rY, rZ) into IT-cache or DT-cache, then RESUME_AGAIN

static void resumeSet(bool isPriv,octa x,int rz);
static void resumeContinue(bool isPriv,tetra raw,int ry,int rz);
static void resumeAgain(bool isPriv,tetra raw);
static void resumeTrans(bool isPriv,tetra raw);

void cpu_instr_trap(const sInstrArgs *iargs) {
	if(cfg_isTestMode() && iargs->x == 0 && iargs->y == 0 && iargs->z == 0)
		cpu_halt();
	else
		ex_throw(EX_FORCED_TRAP,0);
}

void cpu_instr_trip(const sInstrArgs *iargs) {
	UNUSED(iargs);
	// trips are ignored when we're at a negative address
	if(!cpu_isPriv())
		ex_throw(EX_FORCED_TRIP,0);
}

void cpu_instr_resume(const sInstrArgs *iargs) {
	if(iargs->y > 1)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);

	int isPriv = cpu_isPriv();
	int rx,ry,rz,rw;
	// "RESUME 1" at negative addresses uses rXX, rYY and so on
	if(iargs->y == 1 && isPriv) {
		rx = rXX;
		ry = rYY;
		rz = rZZ;
		rw = rWW;
	}
	else {
		rx = rX;
		ry = rY;
		rz = rZ;
		rw = rW;
	}

	// user-apps can't use "RESUME 1" and can't go to privileged addresses
	if(!isPriv) {
		if(iargs->y != 0)
			ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_INSTR);
		if(reg_getSpecial(rw) & MSB(64))
			ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_PC);
	}

	// note: if the MSB is not set so that we should do a RESUME_CONT, RESUME_AGAIN or something,
	// its important that exception-handling is correct.
	// the idea is: if RESUME itself is not correct (e.g. inserted RESUME via RESUME), we have to
	// throw the exception BEFORE setting rK and $255 (see below).
	// but, if the instruction that is inserted via RESUME causes an exception, we have to do that
	// AFTER setting rK and $255. because conceptually the instruction is inserted into the
	// instruction-stream. So, in a sense we're already back in "user-mode". Therefore, rK and $255
	// have to be setup for "user-mode". Otherwise an exception might not be thrown because the bit
	// in rK is not set, for example.

	octa x = reg_getSpecial(rx);
	// first, check if the resume-instruction is correct. throw exception if not
	if(!(x & MSB(64))) {
		tetra raw = x & 0xFFFFFFFF;
		int ropcode = x >> 56;
		switch(ropcode) {
			case RESUME_SET:
				break;

			case RESUME_CONT:
				// not syncd[i] or syncid[i]
				if((OPCODE(raw) & 0xFA) != 0xB8) {
					// 0x8F30 = 1000 1111 0011 0000
					//          FEDC BA98 7654 3210
					// -> we allow instructions that start with hex 0,1,2,3,6,7,C,D,E
					// all of these have either A_DSS or A_DS (or A_III for trap)
					if((1 << (raw >> 28)) & 0x8F30)
						ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
				}
				break;

			case RESUME_AGAIN:
				// inserting resume via resume is no good idea ;)
				if(OPCODE(raw) == RESUME)
					ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
				break;

			case RESUME_TRANS:
				if(iargs->y != 1)
					ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
				break;

			default:
				ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
		}
	}

	// now restore rK from $255 and restore $255 from rBB
	if(iargs->y == 1 && isPriv) {
		// can't throw an exception
		reg_setSpecial(rK,reg_get(255));
		reg_set(255,reg_getSpecial(rBB));
	}
	// first set the new PC; this is necessary for relative jumps and also for exceptions (rXX)
	cpu_setPC(reg_getSpecial(rw) - sizeof(tetra));

	// finally, execute the instruction as if it has been inserted into the instruction-stream
	// before rW
	if(!(x & MSB(64))) {
		tetra raw = x & 0xFFFFFFFF;
		int ropcode = x >> 56;
		switch(ropcode) {
			case RESUME_SET:
				resumeSet(isPriv,x,rz);
				break;

			case RESUME_CONT:
				resumeContinue(isPriv,raw,ry,rz);
				break;

			case RESUME_AGAIN:
				resumeAgain(isPriv,raw);
				break;

			case RESUME_TRANS:
				resumeTrans(isPriv,raw);
				break;
		}
	}
}

static void resumeSet(bool isPriv,octa x,int rz) {
	int dst = (x >> 16) & 0xFF;
	// it behaves like a GET $X,rz
	cpu_setResumeInstr((GET << 24) | (dst << 16) | rz);
	// we need the check for security reasons here: if we're in user mode and set rW to 0, we could
	// execute one instruction in privileged mode because of the -4. rK can't be changed, so that
	// it would raise an exception in this case.
	if(!isPriv && (cpu_getPC() & MSB(64)))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_PC);
	// its not allowed to increase rL here; this way, no exception is possible
	if(dst >= (int)reg_getSpecial(rL) && dst < (int)reg_getSpecial(rG))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	// set $X to rZ and set arith exceptions, if necessary
	reg_set(dst,reg_getSpecial(rz));
	if(x & 0xFF0000000000)
		cpu_setArithEx((x >> 40) & 0xFF);
}

static void resumeContinue(bool isPriv,tetra raw,int ry,int rz) {
	// not syncd[i] or syncid[i]
	if((OPCODE(raw) & 0xFA) != 0xB8) {
		// notify the CPU about the instruction
		cpu_setResumeInstr(raw);
		if(!isPriv && (cpu_getPC() & MSB(64)))
			ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_PC);

		// decode instruction
		sInstrArgs iargs;
		const sInstr *instr = dec_getInstr(OPCODE(raw));
		dec_decode(raw,&iargs);

		// its not allowed to increase rL here
		if(iargs.x >= reg_getSpecial(rL) && iargs.x < reg_getSpecial(rG))
			ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);

		// set operands
		if(instr->args == A_DSS) {
			iargs.y = reg_getSpecial(ry);
			iargs.z = reg_getSpecial(rz);
		}
		// take care of TRAP. its allowed here as well but we won't replace the arguments
		else if(instr->args != A_III)
			iargs.z = reg_getSpecial(rz);

		// execute the instruction; this may throw an exception
		cpu_setResumeInstrArgs(&iargs);
		instr->execute(&iargs);
	}
}

static void resumeAgain(bool isPriv,tetra raw) {
	sInstrArgs iargs;
	dec_decode(raw,&iargs);
	// execute the instruction; this may throw an exception
	const sInstr *instr = dec_getInstr(OPCODE(raw));
	cpu_setResumeInstr(raw);
	cpu_setResumeInstrArgs(&iargs);
	if(!isPriv && (cpu_getPC() & MSB(64)))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_PC);
	instr->execute(&iargs);
}

static void resumeTrans(bool isPriv,tetra raw) {
	octa virt = reg_getSpecial(rYY);
	octa pte = reg_getSpecial(rZZ);
	sVirtTransReg vtr = mmu_unpackrV(reg_getSpecial(rV));
	// SWYM in rXX means: put it in instruction-TC
	int tc = OPCODE(raw) == SWYM ? TC_INSTR : TC_DATA;
	tc_set(tc,tc_addrToKey(virt,vtr.s,vtr.n),tc_pteToTrans(pte,vtr.s));
	// now execute the instruction again (SWYM = NOP here)
	if(OPCODE(raw) != SWYM)
		resumeAgain(isPriv,raw);
	// otherwise, at least set the instruction for the CLI
	else {
		sInstrArgs iargs;
		dec_decode(raw,&iargs);
		cpu_setResumeInstr(raw);
		cpu_setResumeInstrArgs(&iargs);
	}
}
