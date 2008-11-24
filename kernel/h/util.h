/**
 * @version		$Id: util.h 77 2008-11-22 22:27:35Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "../h/common.h"

/**
 * Represents one function-call
 */
typedef struct {
	u32 addr;
	u32 funcAddr;
	cstring funcName;
} sFuncCall;

/* The max. stack-depth getStackTrace() supports */
#define MAX_STACK_DEPTH 100

/**
 * Assembler routine to halt the processor
 */
extern void halt(void);

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outb(u16 port,u8 val);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u8 inb(u16 port);

/**
 * PANIC: Displays the given message and halts
 *
 * @param fmt the format of the message to display
 */
void panic(cstring fmt,...);

/**
 * Builds a stack-trace and returns a pointer to the first element. The addr-field in the
 * struct-array is terminated with 0. Note that it is a static-array!
 *
 * @return a pointer to the first stack-trace-element
 */
sFuncCall *getStackTrace(void);

/**
 * Prints the stack-trace
 */
void printStackTrace(void);

/**
 * Prints the memory from <addr> to <addr> + <dwordCount>
 *
 * @param addr the staring address
 * @param dwordCount the number of dwords to print
 */
void dumpMem(void *addr,u32 dwordCount);

/**
 * Copies <src> to <src> + <count> to <dst>. If the source is not completely mapped
 * the function returns false
 *
 * @param src the address in user-space
 * @param dst the address in kernel-space
 * @param count the number of bytes to copy
 * @return true if successfull
 */
bool copyUserToKernel(u8 *src,u8 *dst,u32 count);

#endif /*UTIL_H_*/
