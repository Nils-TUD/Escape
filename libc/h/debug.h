/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdarg.h>

extern u64 cpu_rdtsc(void);

/**
 * Same as debugf, but with the va_list as argument
 *
 * @param fmt the format
 * @param ap the argument-list
 */
void vdebugf(cstring fmt,va_list ap);

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
void debugf(cstring fmt, ...);

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
void debugString(string s);

#endif /* DEBUG_H_ */
