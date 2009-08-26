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

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdarg.h>

/* the register-offsets used by the macro GET_REGS() */
#define R_EAX		0
#define R_EBX		1
#define R_ECX		2
#define R_EDX		3
#define R_ESI		4
#define R_EDI		5
#define R_ESP		6
#define R_EBP		7
#define R_CS		8
#define R_DS		9
#define R_ES		10
#define R_FS		11
#define R_GS		12
#define R_SS		13
#define R_EFLAGS	14
/* number of registers GET_REGS and PRINT_REGS will need */
#define REG_COUNT	15

/* prints the values in the general-purpose-, segment- and eflags-registers */
#define PRINT_REGS() \
	u32 ___regs[REG_COUNT]; \
	GET_REGS(___regs); \
	printf("eax=0x%08x, ebx=0x%08x, ecx=0x%08x, edx=0x%08x\n", \
			___regs[R_EAX],___regs[R_EBX],___regs[R_ECX],___regs[R_EDX]); \
	printf("esi=0x%08x, edi=0x%08x, esp=0x%08x, ebp=0x%08x\n", \
			___regs[R_ESI],___regs[R_EDI],___regs[R_ESP],___regs[R_EBP]); \
	printf("cs=0x%02x, ds=0x%02x, es=0x%02x, fs=0x%02x, gs=0x%02x, ss=0x%02x, ", \
			___regs[R_CS],___regs[R_DS],___regs[R_ES],___regs[R_FS],___regs[R_GS],___regs[R_SS]); \
	printf("eflags=0x%08x\n",___regs[R_EFLAGS]);

/* writes the register-values in the given buffer */
#define GET_REGS(buffer) \
	GET_REG("eax",buffer[R_EAX]); \
	GET_REG("ebx",buffer[R_EBX]); \
	GET_REG("ecx",buffer[R_ECX]); \
	GET_REG("edx",buffer[R_EDX]); \
	GET_REG("esi",buffer[R_ESI]); \
	GET_REG("edi",buffer[R_EDI]); \
	GET_REG("esp",buffer[R_ESP]); \
	GET_REG("ebp",buffer[R_EBP]); \
	GET_REG("cs",buffer[R_CS]); \
	GET_REG("ds",buffer[R_DS]); \
	GET_REG("es",buffer[R_ES]); \
	GET_REG("fs",buffer[R_FS]); \
	GET_REG("gs",buffer[R_GS]); \
	GET_REG("ss",buffer[R_SS]); \
	__asm__ ( \
		"pushf;" \
		"pop %%eax" \
		: "=a" (buffer[R_EFLAGS]) \
	);

/* writes the value of the register with given name to c */
#define GET_REG(name,c) \
	__asm__ ( \
		"mov %%" name ",%0" \
		: "=a" (c) \
	);

#ifdef __cplusplus
extern "C" {
#endif

extern u64 cpu_rdtsc(void);

/**
 * @return the cpu-cycles of the current thread
 */
u64 getCycles(void);

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
void dbg_stopUTimer(char *prefix);

/**
 * Starts the timer (cpu_rdtsc())
 */
void dbg_startTimer(void);

/**
 * Stops the timer and prints "<prefix>: <clockCycles>\n"
 *
 * @param prefix the prefix for the output
 */
void dbg_stopTimer(char *prefix);

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
void dumpBytes(void *addr,u32 byteCount);

/**
 * Prints <dwordCount> dwords at <addr>
 *
 * @param addr the start-address
 * @param dwordCount the number of dwords
 */
void dumpDwords(void *addr,u32 dwordCount);

/**
 * Prints <byteCount> bytes at <addr> with debugf
 *
 * @param addr the start-address
 * @param byteCount the number of bytes
 */
void debugBytes(void *addr,u32 byteCount);

/**
 * Prints <dwordCount> dwords at <addr> with debugf
 *
 * @param addr the start-address
 * @param dwordCount the number of dwords
 */
void debugDwords(void *addr,u32 dwordCount);

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
void debugUint(u32 n,u8 base);

/**
 * Prints the given integer in base 10
 *
 * @param n the number
 */
void debugInt(s32 n);

/**
 * Prints the given string
 *
 * @param s the string
 */
void debugString(char *s);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H_ */
