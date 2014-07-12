/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Calculates the stacktrace
 *
 * @return the trace (null-terminated)
 */
uintptr_t *getStackTrace(void);

/**
 * Prints the stack-trace
 */
void printStackTrace(void);

/**
 * Determines the program-name of the running process
 *
 * @return the name, statically allocated
 */
const char *getProcName(void);

/**
 * Starts the timer, i.e. reads the current cpu-cycle count for this thread
 */
void dbg_startUTimer(void);

/**
 * Stops the timer, i.e. reads the current cpu-cycle count for this thread, calculates
 * the difference and prints "<prefix>: <cycles>\n".
 *
 * @param prefix the prefix to print
 */
void dbg_stopUTimer(const char *prefix);

/**
 * Starts the timer (rdtsc())
 */
void dbg_startTimer(void);

/**
 * Stops the timer and prints "<prefix>: <clockCycles>\n"
 *
 * @param prefix the prefix for the output
 */
void dbg_stopTimer(const char *prefix);

/**
 * Just intended for debugging. May be used for anything :)
 * It's just a system-call thats used by nothing else, so we can use it e.g. for printing
 * debugging infos in the kernel to points of time controlled by user-apps.
 */
void debug(void);

/**
 * Prints <byteCount> bytes at <addr>
 *
 * @param addr the start-address
 * @param byteCount the number of bytes
 */
void dumpBytes(const void *addr,size_t byteCount);

/**
 * Prints <dwordCount> dwords at <addr>
 *
 * @param addr the start-address
 * @param dwordCount the number of dwords
 */
void dumpDwords(const void *addr,size_t dwordCount);

/**
 * Prints <num> elements each <elsize> big of <array>
 *
 * @param array the array-address
 * @param num the number of elements
 * @param elsize the size of each element
 */
void dumpArray(const void *array,size_t num,size_t elsize);

/**
 * Prints <byteCount> bytes at <addr> with debugf
 *
 * @param addr the start-address
 * @param byteCount the number of bytes
 */
void debugBytes(const void *addr,size_t byteCount);

/**
 * Prints <dwordCount> dwords at <addr> with debugf
 *
 * @param addr the start-address
 * @param dwordCount the number of dwords
 */
void debugDwords(const void *addr,size_t dwordCount);

/**
 * Same as debugf, but with the va_list as argument
 *
 * @param fmt the format
 * @param ap the argument-list
 */
void vdebugf(const char *fmt,va_list ap);

/**
 * Debugging version of printf. Supports the following formatting:
 * %d: signed integer
 * %u: unsigned integer, base 10
 * %o: unsigned integer, base 8
 * %x: unsigned integer, base 16 (small letters)
 * %X: unsigned integer, base 16 (big letters)
 * %b: unsigned integer, base 2
 * %s: string
 * %c: character
 *
 * @param fmt the format
 */
void debugf(const char *fmt, ...);

/**
 * Prints the given unsigned integer in the given base
 *
 * @param n the number
 * @param base the base
 */
void debugUint(uint n,uint base);

/**
 * Prints the given integer in base 10
 *
 * @param n the number
 */
void debugInt(int n);

/**
 * Prints the given string
 *
 * @param s the string
 */
void debugString(char *s);

#if defined(__cplusplus)
}
#endif
