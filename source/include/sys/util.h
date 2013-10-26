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

#pragma once

#include <sys/common.h>
#include <sys/task/thread.h>
#include <sys/ksymbols.h>
#include <stdarg.h>

class Util {
	Util() = delete;

public:
	/**
	 * Represents one function-call
	 */
	struct FuncCall {
		uintptr_t addr;
		uintptr_t funcAddr;
		const char *funcName;
	};

	/* The max. stack-depth getStackTrace() supports */
	static const size_t MAX_STACK_DEPTH		= 100;

	/**
	 * Stores the pagefault information to print it later for debugging purposes
	 *
	 * @param addr the address for which it occurred
	 * @param ip the address where it occurred
	 */
	static void setpf(uintptr_t addr,uintptr_t ip) {
		pfaddr = addr;
		pfip = ip;
	}

	/**
	 * PANIC: Displays the given message and halts
	 *
	 * @param fmt the format of the message to display
	 */
	static void vpanic(const char *fmt,va_list ap);
	static void panic(const char *fmt,...) asm("util_panic");

	/**
	 * Prints the user state of the current thread.
	 *
	 * @param os the output-stream
	 */
	static void printUserState(OStream &os);

	/**
	 * Prints the user state of the given thread
	 *
	 * @param os the output-stream
	 * @param t the thread
	 */
	static void printUserStateOf(OStream &os,const Thread *t);

	/**
	 * Rand will generate a random number between 0 and 'RAND_MAX' (at least 32767).
	 *
	 * @return the random number
	 */
	static int rand();

	/**
	 * Srand seeds the random number generation function rand so it does not produce the same
	 * sequence of numbers.
	 */
	static void srand(uint seed);

	/**
	 * Starts the timer
	 */
	static void startTimer();

	/**
	 * Stops the timer and prints "<prefix>: <instructions>"
	 *
	 * @param os the output-stream
	 * @param prefix the prefix to display
	 */
	static void stopTimer(OStream &os,const char *prefix,...);

	/**
	 * Builds the user-stack-trace for the current thread
	 *
	 * @return the first function-call (for printStackTrace())
	 */
	static FuncCall *getUserStackTrace();

	/**
	 * Builds the user-stack-trace for the given thread
	 *
	 * @param t the thread
	 * @return the first function-call (for printStackTrace()) or NULL if failed
	 */
	static FuncCall *getUserStackTraceOf(Thread *t);

	/**
	 * Builds the kernel-stack-trace of the given thread
	 *
	 * @param t the thread
	 * @return the first function-call (for printStackTrace())
	 */
	static FuncCall *getKernelStackTraceOf(const Thread *t);

	/**
	 * Builds the stack-trace for the kernel
	 *
	 * @return the first function-call (for printStackTrace())
	 */
	static FuncCall *getKernelStackTrace();

	/**
	 * Prints <msg>, followed by a short version of the given stack-trace and a newline.
	 *
	 * @param os the output-stream
	 * @param trace the first function-call (NULL-terminated)
	 * @param msg the message to print before the trace
	 */
	static void printEventTrace(OStream &os,const FuncCall *trace,const char *msg,...);

	/**
	 * Prints the given stack-trace
	 *
	 * @param os the output-stream
	 * @param trace the first function-call (NULL-terminated)
	 */
	static void printStackTrace(OStream &os,const FuncCall *trace);

	/**
	 * Prints the memory from <addr> to <addr> + <dwordCount>
	 *
	 * @param os the output-stream
	 * @param addr the staring address
	 * @param dwordCount the number of dwords to print
	 */
	static void dumpMem(OStream &os,const void *addr,size_t dwordCount);

	/**
	 * Prints <byteCount> bytes at <addr>
	 *
	 * @param os the output-stream
	 * @param addr the start-address
	 * @param byteCount the number of bytes
	 */
	static void dumpBytes(OStream &os,const void *addr,size_t byteCount);

	/**
	 * Switches to VGA mode via video
	 */
	static void switchToVGA();

private:
	/**
	 * Architecture dependent part of panic
	 */
	static void panicArch();

	static uint randa;
	static uint randc;
	static uint lastRand;
	static klock_t randLock;
	static uint64_t profStart;
	static uintptr_t pfaddr;
	static uintptr_t pfip;
};
