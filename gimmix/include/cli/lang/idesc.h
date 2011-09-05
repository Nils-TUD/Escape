/**
 * $Id: idesc.h 217 2011-06-03 18:11:57Z nasmussen $
 */

#ifndef INSTRS_H_
#define INSTRS_H_

#include "common.h"

/**
 * An instruction-description
 */
typedef struct {
	const char *name;
	int mems;
	int oops;
	const char *effect;
} sInstrDesc;

/**
 * @param no the opcode
 * @return the instruction-description for opcode <no>
 */
const sInstrDesc *idesc_get(int no);

/**
 * Disassembles the given instruction
 *
 * @param loc the location of the instruction
 * @param instr the instruction
 * @param absoluteJumps show the absolute addresses for jumps and so on
 * @return the string describing the instruction, statically allocated
 */
const char *idesc_disasm(octa loc,tetra instr,bool absoluteJumps);

#endif /* INSTRS_H_ */
