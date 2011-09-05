/**
 * $Id: cpu.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <assert.h>

#include "common.h"
#include "core/cpu.h"
#include "core/mmu.h"
#include "core/register.h"
#include "core/decoder.h"
#include "dev/timer.h"
#include "mmix/error.h"
#include "exception.h"
#include "sim.h"
#include "event.h"

static void cpu_counterTick(void);
static void cpu_checkForInterrupt(void);
static void cpu_checkSecurity(void);
static void cpu_triggerException(int ex,octa bits);
static void cpu_execInstr(void);

static octa pc;
static bool pcChanged;
static bool run;
static bool halted;
static byte arithEx;
static octa pagefaultAddr;
static octa pagefaultValue;
static octa oldrU;
static int instrCount;

// current instruction
static octa instrLoc;
static bool useResume;
static tetra instrRaw;
static tetra instrRawResume;
static sInstrArgs iargs;
static sInstrArgs iargsResume;
static const sInstr *instr;

static void cpu_sigIntrpt(const sEvArgs *args) {
	UNUSED(args);
	cpu_pause();
}

void cpu_init(void) {
	ev_register(EV_USER_INT,cpu_sigIntrpt);
	cpu_reset();
}

void cpu_reset(void) {
	run = true;
	halted = false;
	arithEx = 0;
	pagefaultAddr = MSB(64);
	pagefaultValue = 0;
	oldrU = 0;
	instrCount = 0;
	pc = 0;
	pcChanged = false;
	useResume = false;
}

octa cpu_getPC(void) {
	return pc;
}

void cpu_setPC(octa npc) {
	pc = npc;
	halted = false;
	pcChanged = true;
}

int cpu_getInstrsSinceTick(void) {
	return instrCount;
}

void cpu_setInstrsSinceTick(int count) {
	instrCount = count;
}

bool cpu_isPriv(void) {
	return pc & MSB(64);
}

bool cpu_hasPCChanged(void) {
	return pcChanged;
}

bool cpu_isPCOk(octa npc) {
	// + sizeof(tetra), because the pc will be increased before it is used
	if(!(pc & MSB(64)) && ((npc + sizeof(tetra)) & MSB(64)))
		return false;
	return true;
}

void cpu_setInterrupt(int irq) {
	reg_setSpecial(rQ,reg_getSpecial(rQ) | ((octa)1 << irq));
}

void cpu_resetInterrupt(int irq) {
	reg_setSpecial(rQ,reg_getSpecial(rQ) & ~((octa)1 << irq));
}

void cpu_setArithEx(octa bits) {
	assert((bits & ~(octa)0xFF) == 0);
	// trips only lead to a throw if the corresponding enable-bit is set
	octa ra = reg_getSpecial(rA);
	// in privileged mode, all trips are implicitly disabled
	if(pc & MSB(64))
		ra &= ~(octa)0xFF00;
	// float-underflow without float-inexact will not be put into the event-bits
	if((bits & (TRIP_FLOATUNDER + TRIP_FLOATINEXACT)) == TRIP_FLOATUNDER &&
			!(ra & (TRIP_FLOATUNDER << 8))) {
		bits &= ~TRIP_FLOATUNDER;
	}
	octa enabled = bits & (ra >> 8);
	if(!(pc & MSB(64)) && enabled) {
		// determine the most-left bit that is enabled
		octa k = 0x80;
		while(!(enabled & k) && k >= 1)
			k >>= 1;
		arithEx |= k;
		// trips taken are not logged as events
		bits &= ~k;
	}
	// log trips not taken
	if(bits)
		reg_setSpecial(rA,reg_getSpecial(rA) | bits);
}

bool cpu_setMemEx(int ex,octa bits,octa addr,octa value,bool sideEffects) {
	if(ex == EX_DYNAMIC_TRAP) {
		bits &= ~TRAP_REPEAT;
		// exception-bits not enabled?
		if(!(bits & reg_getSpecial(rK))) {
			// note: mmix specifies that stores should store nothing in this case and reads that
			// cause a read-protection-fault read zero. we extend that by saying that all failures
			// that occur during stores or reads store nothing or read zero, respectivly.
			if(sideEffects) {
				// provide exception-reason in rQ
				reg_setSpecial(rQ,reg_getSpecial(rQ) | bits);
			}
			return false;
		}
	}
	// store the address and value because we can't determine them easily from the instruction
	if(sideEffects) {
		pagefaultAddr = addr;
		pagefaultValue = value;
	}
	return true;
}

void cpu_step(void) {
	if(!halted) {
		run = true;
		cpu_execInstr();
		ev_fire(EV_CPU_PAUSE);
	}
}

void cpu_run(void) {
	if(!halted) {
		run = true;
		do {
			cpu_execInstr();
		}
		while(run);
		ev_fire(EV_CPU_PAUSE);
	}
}

void cpu_pause(void) {
	run = false;
}

void cpu_halt(void) {
	run = false;
	halted = true;
	ev_fire(EV_CPU_HALTED);
}

bool cpu_isPaused(void) {
	return run == false;
}

bool cpu_isHalted(void) {
	return halted;
}

tetra cpu_getCurInstrRaw(void) {
	return instrRaw;
}

const sInstrArgs *cpu_getCurInstrArgs(void) {
	return &iargs;
}

const sInstr *cpu_getCurInstr(void) {
	return instr;
}

bool cpu_useResume(void) {
	return useResume;
}

void cpu_setResumeInstr(tetra raw) {
	instrRawResume = raw;
	useResume = true;
}

void cpu_setResumeInstrArgs(const sInstrArgs *args) {
	iargsResume.x = args->x;
	iargsResume.y = args->y;
	iargsResume.z = args->z;
}

static void cpu_execInstr(void) {
	// execute the instruction and catch exceptions
	jmp_buf env;
	int ex = setjmp(env);
	if(ex != EX_NONE) {
		if(ex == EX_BREAKPOINT) {
			run = false;
			ex_pop();
			return;
		}
		else
			cpu_triggerException(ex,ex_getBits());
	}
	else {
		ex_push(&env);

		// fetch instruction
		ev_fire(EV_BEFORE_FETCH);
		pcChanged = false;
		useResume = false;
		instrLoc = pc;
		instrRaw = mmu_readInstr(pc,MEM_SIDE_EFFECTS);

		// decode
		instr = dec_getInstr(OPCODE(instrRaw));
		cpu_checkSecurity();
		dec_decode(instrRaw,&iargs);

		// execute
		ev_fire(EV_BEFORE_EXEC);
		instr->execute(&iargs);

		// handle realtime tasks
		if(++instrCount == INSTRS_PER_TICK) {
			instrCount = 0;
			timer_tick();
		}
		// check for interrupt-request
		cpu_checkForInterrupt();

		// tick the counters here to count only instructions that have been executed completely
		cpu_counterTick();

		// don't fire EV_AFTER_EXEC here because the instruction may throw an exception
		// in this case we would not reach this
	}
	ex_pop();

	// increase cycle-counter (even if we've not really executed the instruction because of an
	// exception. but doing it always is better because this way stepping 10000 times means that
	// rC is 10000 afterwards)
	reg_setSpecial(rC,reg_getSpecial(rC) + 1);

	// fire the event before pc-change to be able to detect positive branch-results
	// although we can't see when we're branching to the next instruction. but this makes no
	// sense, therefore its better than not detecting a jump to the same instr
	ev_fire(EV_AFTER_EXEC);

	// go to next instruction
	pc += sizeof(tetra);
}

static void cpu_counterTick(void) {
	// increase usage-counter; use the old value of rU to determine whether we should do something.
	// this way we do it in the same way mmmix does.
	octa usage = reg_getSpecial(rU);
	byte upattern = oldrU >> 56;
	byte umask = (oldrU >> 48) & 0xFF;
	if((OPCODE(instrRaw) & umask) == upattern) {
		// instructions in privileged mode are counted only if bit 47 is set
		if(!(instrLoc & MSB(64)) || (oldrU & 0x800000000000)) {
			// increase the new usage-value and take care of overflow
			if((usage & 0x7FFFFFFFFFFF) == 0x7FFFFFFFFFFF)
				usage -= 0x7FFFFFFFFFFF;
			else
				usage++;
			reg_setSpecial(rU,usage);
		}
	}
	oldrU = usage;
}

static void cpu_checkForInterrupt(void) {
	// decrease interval-counter
	octa ri = reg_getSpecial(rI) - 1;
	reg_setSpecial(rI,ri);
	if(ri == 0)
		reg_setSpecial(rQ,reg_getSpecial(rQ) | TRAP_INTERVAL);

	// check for interrupt requests
	octa irqs = reg_getSpecial(rQ) & reg_getSpecial(rK);
	if(irqs != 0) {
		// filter out exceptions because they are not provided in rX in this case
		// (they are only put in rX if the interrupted instruction caused them)
		ex_throw(EX_DYNAMIC_TRAP,irqs & ~0xFF00000000);
	}
	else if(!(pc & MSB(64)) && arithEx) {
		unsigned bits = arithEx;
		arithEx = 0;
		ex_throw(EX_DYNAMIC_TRIP,bits);
	}
}

static void cpu_checkSecurity(void) {
	if(pc & MSB(64)) {
		// in kernel-mode P_BIT has to be unset
		if(reg_getSpecial(rK) & TRAP_PRIVILEGED_PC)
			ex_throw(EX_DYNAMIC_TRAP,TRAP_PRIVILEGED_PC);
	}
	else {
		// in user-mode all program bits have to be set
		if(((reg_getSpecial(rK) >> 32) & 0xFF) != 0xFF) {
			// first set these bits, otherwise the exception isn't thrown
			// rK will be set to zero anyway
			reg_setSpecial(rK,reg_getSpecial(rK) | 0xFF00000000);
			ex_throw(EX_DYNAMIC_TRAP,TRAP_SECURITY_VIOLATION);
		}
	}
}

static void cpu_triggerException(int ex,octa bits) {
	octa rxVal;

	// we have to repeat the instruction, if it occurred during fetch
	if(bits & TRAP_PROT_EXEC)
		pc -= sizeof(tetra);
	// set ropcode RESUME_TRANS if software-translation is desired
	if(ex == EX_FORCED_TRAP && (bits & TRAP_SOFT_TRANS))
		rxVal = (octa)3 << 56;
	// set ropcode RESUME_AGAIN
	else if(ex == EX_DYNAMIC_TRAP && (bits & TRAP_REPEAT)) {
		bits &= ~TRAP_REPEAT;
		rxVal = 0;
	}
	// otherwise skip instruction
	else
		rxVal = MSB(64);

	// put instruction in rX
	// if its a exec-protection-fault, use SWYM to indicate a noop for RESUME_TRANS
	if(bits & TRAP_PROT_EXEC)
		rxVal |= (octa)SWYM << 24;
	// default case: the current instruction
	else
		rxVal |= useResume ? instrRawResume : instrRaw;

	if((bits & 0xFF00000000) && ex == EX_DYNAMIC_TRAP) {
		// provide exception-reason in rQ
		reg_setSpecial(rQ,reg_getSpecial(rQ) | bits);
		// if the exception-bit is not enabled, don't trap
		if(!(reg_getSpecial(rQ) & reg_getSpecial(rK)))
			return;
		rxVal |= bits & 0xFF00000000;
	}

	// note: its fired here because we don't want to show exceptions that aren't triggered because
	// the bit in rK is not set
	ev_fire2(EV_EXCEPTION,ex,bits);

	// traps use rXX, rYY and so on
	int rx,ry,rz,rb,rw,rt;
	if(ex == EX_DYNAMIC_TRAP || ex == EX_FORCED_TRAP) {
		rx = rXX;
		ry = rYY;
		rz = rZZ;
		rb = rBB;
		rw = rWW;
	}
	else {
		rx = rX;
		ry = rY;
		rz = rZ;
		rb = rB;
		rw = rW;
	}
	rt = ex == EX_DYNAMIC_TRAP ? rTT : rT;

	// arg...the rest of a forced software-translation behaves like a dynamic trap
	if(bits & TRAP_SOFT_TRANS)
		ex = EX_DYNAMIC_TRAP;

	// set rX/rXX to the instruction and set the MSB so that we continue with the instruction
	// at g[rW] afterwards (at least in the default case)
	reg_setSpecial(rx,rxVal);
	// set rW to the next instruction
	reg_setSpecial(rw,pc + sizeof(tetra));

	// put the operands in rY,rZ
	switch(ex) {
		case EX_DYNAMIC_TRAP:
		case EX_DYNAMIC_TRIP:
			// if a page-fault occurred, provide that address in rY and the value that should
			// be stored in rZ (0 for loads)
			if(!(pagefaultAddr & MSB(64))) {
				reg_setSpecial(ry,pagefaultAddr);
				reg_setSpecial(rz,pagefaultValue);
				pagefaultAddr = MSB(64);
				pagefaultValue = 0;
			}
			else {
				reg_setSpecial(ry,useResume ? iargsResume.y : iargs.y);
				reg_setSpecial(rz,useResume ? iargsResume.z : iargs.z);
			}
			break;

		case EX_FORCED_TRAP:
		case EX_FORCED_TRIP:
			reg_setSpecial(ry,reg_get(useResume ? iargsResume.y : iargs.y));
			reg_setSpecial(rz,reg_get(useResume ? iargsResume.z : iargs.z));
			break;
	}

	// set rB to $255 and $255 to rJ
	reg_setSpecial(rb,reg_get(255));
	reg_set(255,reg_getSpecial(rJ));

	// clear rK (inhibit interrupts)
	if(ex == EX_DYNAMIC_TRAP || ex == EX_FORCED_TRAP)
		reg_setSpecial(rK,0);

	// now set the new pc (take care that we'll increase it by 4 afterwards)
	switch(ex) {
		case EX_DYNAMIC_TRAP:
		case EX_FORCED_TRAP:
			pc = reg_getSpecial(rt) - sizeof(tetra);
			break;

		case EX_DYNAMIC_TRIP:
		case EX_FORCED_TRIP:
			pc = -(octa)sizeof(tetra);
			if(bits) {
				// note: we're using the first bit that is set, starting on the left
				// but since arithmetic exceptions are handled by cpu_setArithEx, there is just
				// one bit enabled anyway. so, priority has already been considered.
				pc += 16;
				for(int k = 128; k > 0 && !(bits & k); k >>= 1)
					pc += 16;
			}
			break;
	}
}
