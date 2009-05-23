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

#include "common.h"
#include "ksymbols.h"
#include "intrpt.h"
#include "thread.h"
#include <stdarg.h>

/**
 * Represents one function-call
 */
typedef struct {
	u32 addr;
	u32 funcAddr;
	const char *funcName;
} sFuncCall;

/**
 * The buffer for util_(v)sprintf()
 */
typedef struct {
	u8 dynamic;
	char *str;
	u32 size;
	u32 len;
} sStringBuffer;

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
 * @param t the thread
 * @param stack the interrupt-stack
 * @return the first function-call (for util_printStackTrace())
 */
sFuncCall *util_getUserStackTrace(sThread *t,sIntrptStackFrame *stack);

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
 * Determines the required length for the given format and arguments
 *
 * @param fmt the format
 * @return the required length of the string util_sprintf() would write to
 */
u32 util_sprintfLen(const char *fmt,...);

/**
 * Determines the required length for the given format and arguments
 *
 * @param fmt the format
 * @param ap the argument-list
 * @return the required length of the string util_sprintf() would write to
 */
u32 util_vsprintfLen(const char *fmt,va_list ap);

/**
 * Kernel-version of sprintf
 *
 * @param buf the buffer to use. if buf->dynamic is true, sprintf() manages the memory-allocation.
 * 	if buf->str is NULL it will be allocated at beginning, otherwise the existing buffer will
 *	be used.
 * @param fmt the format
 * @return true on success (can just fail if buf->dynamic is true)
 */
bool util_sprintf(sStringBuffer *buf,const char *fmt,...);

/**
 * Kernel-version of vsprintf
 *
 * @param buf the buffer to use. if buf->dynamic is true, sprintf() manages the memory-allocation.
 * 	if buf->str is NULL it will be allocated at beginning, otherwise the existing buffer will
 *	be used.
 * @param fmt the format
 * @param ap the argument-list
 * @return true on success (can just fail if buf->dynamic is true)
 */
bool util_vsprintf(sStringBuffer *buf,const char *fmt,va_list ap);

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
