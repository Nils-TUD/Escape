/**
 * $Id: exception.h 172 2011-04-15 09:41:38Z nasmussen $
 */

#ifndef ENV_H_
#define ENV_H_

#include <setjmp.h>
#include <assert.h>
#include "common.h"

/**
 * This module is used to provide a rudimentary exception-handling in C. It uses setjmp and longjmp
 * and a stack of "environments" to achieve that. It is used both for the CPU and for the CLI.
 * The environment-stack is used to allow nested "try-and-catch".
 *
 * Example:
 * jmp_buf env;
 * int ex = setjmp(env);
 * if(ex != EX_NONE) {
 *	// handle exception
 * }
 * else {
 * 	ex_push(&env);
 *  // do something that might throw an exception
 * }
 * // note that this is necessary in all cases, i.e. even if an exception has been catched.
 * ex_pop();
 */

#define EX_NONE					0
#define EX_FORCED_TRIP			1			// trip-instructions
#define EX_DYNAMIC_TRIP			2			// arithmetic exceptions
#define EX_FORCED_TRAP			3			// trap-instructions
#define EX_DYNAMIC_TRAP			4			// device-interrupts and exceptions
#define EX_CLI					5			// exceptions in the CLI
#define EX_BREAKPOINT			6			// breakpoint-exception

#define TRAP_POWER_FAILURE		(1 << 0)	// power failure
#define TRAP_MEMORY_PARITY		(1 << 1)	// memory parity error
#define TRAP_NONEX_MEMORY		(1 << 2)	// non-existent memory
#define TRAP_REBOOT				(1 << 3)	// reboot
#define TRAP_INTERVAL			(1 << 7)	// interval-counter reached zero

#define TRAP_PROT_READ			((octa)1 << 39)	// r: tries to load from a page without read perm
#define TRAP_PROT_WRITE			((octa)1 << 38)	// w: tries to store to a page without write perm
#define TRAP_PROT_EXEC			((octa)1 << 37)	// x: appears in a page without execute permission
#define TRAP_PRIVILEGED_ACCESS	((octa)1 << 36)	// n: refers to a negative virtual address
#define TRAP_PRIVILEGED_INSTR	((octa)1 << 35)	// k: is privileged, for use by the "kernel" only
#define TRAP_BREAKS_RULES		((octa)1 << 34)	// b: breaks the rules of MMIX
#define TRAP_SECURITY_VIOLATION	((octa)1 << 33)	// s: violates security
#define TRAP_PRIVILEGED_PC		((octa)1 << 32)	// p: comes from a privileged (negative) virt addr
#define TRAP_REPEAT				((octa)1 << 31)	// NOT official! Causes GIMMIX to repeat an instr
#define TRAP_SOFT_TRANS			((octa)1 << 30)	// NOT official! Requests software-translation

// the exception bits in rQ
#define TRAP_EX_BITS			(TRAP_POWER_FAILURE | TRAP_MEMORY_PARITY | TRAP_NONEX_MEMORY | \
								 TRAP_REBOOT | TRAP_INTERVAL | TRAP_PRIVILEGED_PC | \
								 TRAP_SECURITY_VIOLATION | TRAP_BREAKS_RULES | \
								 TRAP_PRIVILEGED_INSTR | TRAP_PRIVILEGED_ACCESS | \
								 TRAP_PROT_EXEC | TRAP_PROT_WRITE | TRAP_PROT_READ)
#define TRAP_RWX_BITS			(TRAP_PROT_EXEC | TRAP_PROT_WRITE | TRAP_PROT_READ)

#define TRIP_FLOATINEXACT		(1 << 0)	// X_BIT
#define TRIP_FLOATDIVBY0		(1 << 1)	// Z_BIT
#define TRIP_FLOATUNDER			(1 << 2)	// U_BIT
#define TRIP_FLOATOVER			(1 << 3)	// O_BIT
#define TRIP_FLOATINVOP			(1 << 4)	// I_BIT
#define TRIP_FLOAT2FIXOVER		(1 << 5)	// W_BIT
#define TRIP_INTOVER			(1 << 6)	// V_BIT
#define TRIP_INTDIVCHK			(1 << 7)	// D_BIT

#define TRIP_RA_OFFSET			8			// offset in rA

#define MAX_ENV_NEST_DEPTH		10

extern jmp_buf *envs[];
extern int curEnv;
extern int curEx;
extern octa curBits;

/**
 * Converts the given exception and exception-bits to string for debugging.
 *
 * @param exception the exception (EX_*)
 * @param bits for EX_DYNAMIC_*: the corresponding bits of TRAP_* or TRIP*
 * @return the string-representation
 */
const char *ex_toString(int exception,octa bits);

/**
 * Throws the given exception with given bits
 *
 * @param exception the exception (EX_*)
 * @param bits for EX_DYNAMIC_*: the corresponding bits of TRAP_* or TRIP*
 */
void ex_throw(int exception,octa bits);

/**
 * Rethrows the current exception
 */
void ex_rethrow(void);

/**
 * @return the last thrown exception. will be reset by ex_push()
 */
static inline int ex_getEx(void) {
	return curEx;
}

/**
 * @return the bits of the last thrown exception. will be reset by ex_push()
 */
static inline octa ex_getBits(void) {
	return curBits;
}

/**
 * Pushes a new environment on the environment stack. If ex_throw() is used, it restores
 * the given environment.
 *
 * @param environment the environment
 */
static inline void ex_push(jmp_buf *environment) {
	assert(curEnv < MAX_ENV_NEST_DEPTH - 1);
	curEx = 0;
	curBits = 0;
	curEnv++;
	envs[curEnv] = environment;
}

/**
 * Pops an environment from the stack. This is required after each push, regardless of whether
 * an exception has been thrown & catched or not.
 */
static inline void ex_pop(void) {
	assert(curEnv >= 0);
	curEnv--;
}

#endif /* ENV_H_ */
