/**
 * @version		$Id: debug.c 51 2008-11-15 09:50:24Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef DEBUG_H_
#define DEBUG_H_

/**
 * Debugging version of printf. Supports the following formatting:
 * %d
 * %u
 * %b
 * %x
 * %o
 * %s
 * %c
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
