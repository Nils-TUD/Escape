/**
 * $Id$
 */

#ifndef ECO32_DEBUG_H_
#define ECO32_DEBUG_H_

#include <esc/common.h>
#include <stdarg.h>

/**
 * Prints the given char
 *
 * @param c the character
 */
extern void debugc(char c);

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
void debugu(uint n,uint base);

/**
 * Prints the given integer in base 10
 *
 * @param n the number
 */
void debugi(int n);

/**
 * Prints the given string
 *
 * @param s the string
 */
void debugs(const char *s);

#endif /* ECO32_DEBUG_H_ */
