/**
 * $Id: register.h 207 2011-05-14 11:55:06Z nasmussen $
 */

#ifndef REGISTER_H_
#define REGISTER_H_

#include "common.h"
#include "config.h"
#include <assert.h>

#define LREG_MASK		(LREG_NUM - 1)
#define GREG_MASK		(256 - 1)		// we have always 256 slots

#define MIN_GLOBAL		32				// min. value of rG
#define SPECIAL_NUM		33				// number of special registers

#define rA				21				// arithmetic status register
#define rB				0				// bootstrap register (trip)
#define rC				8				// cycle counter
#define rD				1				// dividend register
#define rE				2				// epsilon register
#define rF				22				// failure location register
#define rG				19				// global threshold register
#define rH				3				// himult register
#define rI				12				// interval register
#define rJ				4				// return-jump register
#define rK				15				// interrupt mask register
#define rL				20				// local threshold register
#define rM				5				// multiplex mask register
#define rN				9				// serial number
#define rO				10				// register stack offset
#define rP				23				// prediction register
#define rQ				16				// interrupt request register
#define rR				6				// remainder register
#define rS				11				// register stack pointer
#define rT				13				// trap address register
#define rU				17				// usage counter
#define rV				18				// virtual translation register
#define rW				24				// where-interrupted register (trip)
#define rX				25				// execution register (trip)
#define rY				26				// Y operand (trip)
#define rZ				27				// Z operand (trip)
#define rBB				7				// bootstrap register (trap)
#define rTT				14				// dynamic trap address register
#define rWW				28				// where interrupted register (trap)
#define rXX				29				// execution register (trap)
#define rYY				30				// Y operand (trap)
#define rZZ				31				// Z operand (trap)
#define rSS				32				// kernel-stack location (GIMMIX only)

// the types for the load/store events
#define RSTACK_DEFAULT	0
#define RSTACK_GREGS	1
#define RSTACK_SPECIAL	2
#define RSTACK_RGRA		3

extern int L;
extern int G;
extern octa O;
extern octa S;
extern octa local[];
extern octa global[];
extern octa special[];

/**
 * Initializes the registers
 */
void reg_init(void);

/**
 * Resets the registers
 */
void reg_reset(void);

/**
 * Returns l[<rno>]
 *
 * @param rno the register-number
 * @return the value of the local register <rno>
 */
static inline octa reg_getLocal(int rno) {
	return local[rno & LREG_MASK];
}

/**
 * Sets l[<rno>] to value
 *
 * @param rno the register-number
 * @param value the new value
 */
static inline void reg_setLocal(int rno,octa value) {
	local[rno & LREG_MASK] = value;
}

/**
 * Returns g[<rno>]
 *
 * @param rno the register-number
 * @return the value of the global register <rno>
 */
static inline octa reg_getGlobal(int rno) {
	assert(rno >= MAX(256 - GREG_NUM,MIN_GLOBAL));
	return global[rno & GREG_MASK];
}

/**
 * Sets g[<rno>] to value
 *
 * @param rno the register-number
 * @param value the new value
 */
static inline void reg_setGlobal(int rno,octa value) {
	assert(rno >= MAX(256 - GREG_NUM,MIN_GLOBAL));
	global[rno & GREG_MASK] = value;
}

/**
 * Returns the value of the special-register with given number
 *
 * @param rno the register-number: rA,rB,...
 * @return the value
 */
static inline octa reg_getSpecial(int rno) {
	assert(rno < SPECIAL_NUM);
	return special[rno];
}

/**
 * Sets the value of the special-register with given number
 *
 * @param rno the register-number: rA,rB,...
 * @param value the new value
 */
static inline void reg_setSpecial(int rno,octa value) {
	assert(rno < SPECIAL_NUM);
	special[rno] = value;
	// update our shortcuts
	if(rno == rG)
		G = value;
	else if(rno == rL)
		L = value;
	else if(rno == rS)
		S = value / sizeof(octa);
	else if(rno == rO)
		O = value / sizeof(octa);
}

/**
 * Returns $<rno>
 *
 * @param rno the register-number
 * @return the value of the dynamic register <rno>
 */
static inline octa reg_get(int rno) {
	if(rno >= G)
		return global[rno & GREG_MASK];
	if(rno < L)
		return local[(O + rno) & LREG_MASK];
	return 0;
}

/**
 * Sets $<rno> to value
 *
 * @param rno the register-number
 * @param value the new value
 * @throws any kind of exception that can occur when writing to memory, if rno >= rL and the
 *  register-ring is full
 */
void reg_set(int rno,octa value);

/**
 * Performs a push in the sense of the MMIX-instruction PUSHJ/PUSHGO.
 * Note that it does not change rJ and does not change the PC.
 *
 * @param rno the hole-register (all registers before that are used as arguments)
 * @throws any kind of exception that can occur when writing to memory, if rno >= rL and the
 *  register-ring is full
 */
void reg_push(int rno);

/**
 * Performs a pop in the sense of the MMIX-instruction POP
 * Note that it does not change the PC.
 *
 * @param rno the first register not to use as return-value
 * @throws any kind of exception that can occur when reading from memory, if some registers have
 *  to be restored from memory to make sure all local registers of the function we return to are
 *  present
 */
void reg_pop(int rno);

/**
 * Performs a save in the sense of the MMIX-instruction SAVE
 *
 * @param dst the global destination register that should receive the address
 * @param changeStack whether to change rS/rO to rSS
 * @throws any kind of exception that can occur when writing to memory
 */
void reg_save(int dst,bool changeStack);

/**
 * Performs an unsave in the sense of the MMIX-instruction UNSAVE
 *
 * @param src the address to load the values from
 * @param changeStack whether to change rS/rO to the stored values on the stack
 * @throws any kind of exception that can occur when reading from memory
 * @throws TRAP_BREAKS_RULES if the rG-rA value to be loaded from the stack is not valid
 */
void reg_unsave(octa src,bool changeStack);

#endif /* REGISTER_H_ */
