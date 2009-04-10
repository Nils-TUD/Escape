/**
 * @version		$Id: util.h 77 2008-11-22 22:27:35Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"
#include "ksymbols.h"
#include "intrpt.h"
#include "proc.h"
#include <stdarg.h>

/**
 * Represents one function-call
 */
typedef struct {
	u32 addr;
	u32 funcAddr;
	const char *funcName;
} sFuncCall;

/* The max. stack-depth util_getStackTrace() supports */
#define MAX_STACK_DEPTH 100

/**
 * Assembler routine to halt the processor
 */
extern void util_halt(void);

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void util_outByte(u16 port,u8 val);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u8 util_inByte(u16 port);

/**
 * @return the address of the stack-frame-start
 */
extern u32 getStackFrameStart(void);

/**
 * PANIC: Displays the given message and halts
 *
 * @param fmt the format of the message to display
 */
void util_panic(const char *fmt,...);

/**
 * Builds the stack-trace for a user-app
 *
 * @param p the process
 * @param stack the interrupt-stack
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getUserStackTrace(sProc *p,sIntrptStackFrame *stack);

/**
 * Builds the stack-trace for the kernel
 *
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getKernelStackTrace(void);

/**
 * Builds the stacktrace with given vars
 *
 * @param ebp the current value of ebp
 * @param start the stack-start
 * @param end the stack-end
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getStackTrace(u32 *ebp,u32 start,u32 end);

/**
 * Prints the given stack-trace
 *
 * @param trace the first function-call (NULL-terminated)
 */
void util_printStackTrace(sFuncCall *trace);

/**
 * Prints the memory from <addr> to <addr> + <dwordCount>
 *
 * @param addr the staring address
 * @param dwordCount the number of dwords to print
 */
void util_dumpMem(void *addr,u32 dwordCount);

/**
 * Prints <byteCount> bytes at <addr>
 *
 * @param addr the start-address
 * @param byteCount the number of bytes
 */
void util_dumpBytes(void *addr,u32 byteCount);

/**
 * Kernel-version of sprintf
 *
 * @param str the string
 * @param fmt the format
 */
void util_sprintf(char *str,const char *fmt,...);

/**
 * Kernel-version of vsprintf
 *
 * @param str the string
 * @param fmt the format
 * @param ap the argument-list
 */
void util_vsprintf(char *str,const char *fmt,va_list ap);

/**
 * Determines the width of the given signed 32-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
u8 util_getnwidth(s32 n);

/**
 * Determines the width of the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
u8 util_getuwidth(u32 n,u8 base);

#endif /*UTIL_H_*/
