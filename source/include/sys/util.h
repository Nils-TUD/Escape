/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#ifdef __i386__
#include <sys/arch/i586/util.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/util.h>
#endif

/* The max. stack-depth util_getStackTrace() supports */
#define MAX_STACK_DEPTH 100

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
sFuncCall *util_getKernelStackTraceOf(const sThread *t);

/**
 * Builds the stack-trace for the kernel
 *
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getKernelStackTrace(void);

/**
 * Prints a short version of the given stack-trace
 *
 * @param trace the first function-call (NULL-terminated)
 */
void util_printStackTraceShort(const sFuncCall *trace);

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
