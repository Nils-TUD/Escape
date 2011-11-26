/**
 * $Id: cpu.h 239 2011-08-30 18:28:06Z nasmussen $
 */

#ifndef CPU_H_
#define CPU_H_

#include "common.h"
#include "core/decoder.h"

#define MASK(n)				((1ULL << (n)) - 1)
#define MSB(n)				(1ULL << ((n) - 1))

#define ZEXT(x,n)			((x) & MASK(n))
#define SEXT(x,n)			(((x) & MSB(n)) ? ((x) | ~MASK(n)) : ((x) & MASK(n)))

extern octa pc;
extern bool pcChanged;
extern bool halted;

/**
 * Initializes the CPU
 */
void cpu_init(void);

/**
 * Resets the CPU
 */
void cpu_reset(void);

/**
 * @return the current program counter
 */
static inline octa cpu_getPC(void) {
	return pc;
}

/**
 * Sets the PC to <npc>. Unchecked!
 *
 * @param npc the new pc
 */
static inline void cpu_setPC(octa npc) {
	pc = npc;
	halted = false;
	pcChanged = true;
}

/**
 * @return true if the CPU is in privileged mode
 */
static inline bool cpu_isPriv(void) {
	return pc & MSB(64);
}

/**
 * @return true if the last instruction has changed the PC
 */
static inline bool cpu_hasPCChanged(void) {
	return pcChanged;
}

/**
 * Checks wether <npc> is OK (depending on the current pc)
 *
 * @param npc the new pc
 * @return true if ok
 */
static inline bool cpu_isPCOk(octa npc) {
	// + sizeof(tetra), because the pc will be increased before it is used
	if(!(pc & MSB(64)) && ((npc + sizeof(tetra)) & MSB(64)))
		return false;
	return true;
}

/**
 * @return the number of instructions that have been executed since the last tick
 */
int cpu_getInstrsSinceTick(void);

/**
 * Sets the number of instructions that have been executed since the last tick.
 *
 * @param count the new value
 */
void cpu_setInstrsSinceTick(int count);

/**
 * Sets the bit <irq> in rQ. That means it ORs (1 << <irq>) into rQ.
 *
 * @param irq the bit to set
 */
void cpu_setInterrupt(int irq);

/**
 * Unsets the bit <irq> in rQ. That means it ANDs rQ with ~(1 << <irq>).
 *
 * @param irq the bit to unset
 */
void cpu_resetInterrupt(int irq);

/**
 * Raises an arithmetic exception for <bits> if these bits are enabled in rA.
 *
 * @param bits the exception-bits
 */
void cpu_setArithEx(octa bits);

/**
 * Reports a memory-exception for <ex> and <bits>. If it is one and an exception should be
 * raised, it returns true. If necessary, it sets the corresponding bit in rQ.
 *
 * @param ex the exception
 * @param bits the exception-bits
 * @param addr the address for which the fault occurred
 * @param value the value to be stored (0 for loads)
 * @param sideEffects whether the state of the machine should be changed
 * @return true if an exception should be raised
 */
bool cpu_setMemEx(int ex,octa bits,octa addr,octa value,bool sideEffects);

/**
 * Executes one instruction
 */
void cpu_step(void);

/**
 * Executes instructions until a breakpoint is reached or the CPU has been halted.
 */
void cpu_run(void);

/**
 * Pauses the CPU because a breakpoint has been reached or similar.
 */
void cpu_pause(void);

/**
 * Halts the CPU
 */
void cpu_halt(void);

/**
 * @return whether the CPU has been paused
 */
bool cpu_isPaused(void);

/**
 * @return whether the CPU has been halted
 */
bool cpu_isHalted(void);

/**
 * @return the current instruction, in raw format
 */
tetra cpu_getCurInstrRaw(void);

/**
 * @return the current instruction-arguments
 */
const sInstrArgs *cpu_getCurInstrArgs(void);

/**
 * @return the description of the current instruction
 */
const sInstr *cpu_getCurInstr(void);

/**
 * @return whether the instruction to be resumed, has been already started
 */
bool cpu_useResume(void);

/**
 * This is intended for resume only, because it may execute other instructions, depending on ropcode.
 * This way, we can set a useful value to rXX and so on.
 *
 * @param raw the raw instruction
 */
void cpu_setResumeInstr(tetra raw);

/**
 * This is intended for resume only, because it may execute other instructions, depending on ropcode.
 * This way, we can set a useful value to rYY and rZZ.
 *
 * @param args the arguments
 */
void cpu_setResumeInstrArgs(const sInstrArgs *args);

#endif /* CPU_H_ */
