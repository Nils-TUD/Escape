/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRINT_H_
#define PRINT_H_

#include "common.h"
#include <stdarg.h>

/**
 * Prints the given character to the current position on the screen
 *
 * @param c the character
 */
void putchar(s8 c);

/**
 * Reads one character from the vterm
 *
 * @return the character
 */
s8 readChar(void);

/**
 * Reads a line from the vterm
 *
 * @param buffer the buffer to use
 * @param max the maximum number of chars to read
 * @return the number of read chars
 */
u16 readLine(s8 *buffer,u16 max);

/**
 * The kernel-version of printf. Currently it supports:
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
void printf(cstring fmt,...);

/**
 * Same as printf, but with the va_list as argument
 *
 * @param fmt the format
 * @param ap the argument-list
 */
void vprintf(cstring fmt,va_list ap);

#endif /* PRINT_H_ */
