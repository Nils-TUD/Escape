/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <common.h>
#include <machine/intrpt.h>
#include <task/thread.h>
#include <ksymbols.h>
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
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void util_outWord(u16 port,u16 val);

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void util_outDWord(u16 port,u32 val);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u8 util_inByte(u16 port);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u16 util_inWord(u16 port);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u32 util_inDWord(u16 port);

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
 * Waits until any key is pressed (make-code)
 *
 * @return the scancode
 */
u8 util_waitForKeyPress(void);

/**
 * Waits until any key is released (break-code)
 *
 * @return the scancode
 */
u8 util_waitForKeyRelease(void);

/**
 * Starts the timer
 */
void util_startTimer(void);

/**
 * Stops the timer and displays "<prefix>: <instructions>"
 *
 * @param prefix the prefix to display
 */
void util_stopTimer(const char *prefix,...);

/**
 * Builds the user-stack-trace for the current thread
 *
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getUserStackTrace(void);

/**
 * Builds the user-stack-trace for the given thread
 *
 * @param t the thread
 * @return the first function-call (for util_printStackTrace()) or NULL if failed
 */
sFuncCall *util_getUserStackTraceOf(sThread *t);

/**
 * Builds the kernel-stack-trace of the given thread
 *
 * @param t the thread
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getKernelStackTraceOf(sThread *t);

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
 * @param rstart the stack-start (the real address)
 * @param rend the stack-end (the real address)
 * @param mstart the stack-start (the mapped address)
 * @param mend the stack-end (the mapped address)
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getStackTrace(u32 *ebp,u32 rstart,u32 rend,u32 mstart,u32 mend);

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

#endif /*UTIL_H_*/
