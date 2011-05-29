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

#include <sys/common.h>
#include <sys/intrpt.h>
#include <sys/task/thread.h>
#include <sys/ksymbols.h>
#include <stdarg.h>

/**
 * Represents one function-call
 */
typedef struct {
	uintptr_t addr;
	uintptr_t funcAddr;
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
extern void util_outByte(uint16_t port,uint8_t val);

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void util_outWord(uint16_t port,uint16_t val);

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void util_outDWord(uint16_t port,uint32_t val);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint8_t util_inByte(uint16_t port);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint16_t util_inWord(uint16_t port);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint32_t util_inDWord(uint16_t port);

/**
 * @return the address of the stack-frame-start
 */
extern uintptr_t getStackFrameStart(void);

/**
 * PANIC: Displays the given message and halts
 *
 * @param fmt the format of the message to display
 */
void util_panic(const char *fmt,...);

/**
 * Starts the log-viewer
 */
void util_logViewer(void);

/**
 * Rand will generate a random number between 0 and 'RAND_MAX' (at least 32767).
 *
 * @return the random number
 */
int util_rand(void);

/**
 * Srand seeds the random number generation function rand so it does not produce the same
 * sequence of numbers.
 */
void util_srand(uint seed);

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
sFuncCall *util_getUserStackTraceOf(const sThread *t);

/**
 * Builds the kernel-stack-trace of the given thread
 *
 * @param t the thread
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getKernelStackTraceOf(const sThread *t);

/**
 * Builds the stack-trace for the kernel
 *
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getKernelStackTrace(void);

/**
 * Prints the given stack-trace
 *
 * @param trace the first function-call (NULL-terminated)
 */
void util_printStackTrace(const sFuncCall *trace);

/**
 * Prints the memory from <addr> to <addr> + <dwordCount>
 *
 * @param addr the staring address
 * @param dwordCount the number of dwords to print
 */
void util_dumpMem(const void *addr,size_t dwordCount);

/**
 * Prints <byteCount> bytes at <addr>
 *
 * @param addr the start-address
 * @param byteCount the number of bytes
 */
void util_dumpBytes(const void *addr,size_t byteCount);

#endif /*UTIL_H_*/
