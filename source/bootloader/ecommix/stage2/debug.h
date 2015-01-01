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

#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Prints a dump of <byteCount> bytes at <addr>.
 *
 * @param addr the start-address
 * @param byteCount the number of bytes
 */
void dumpBytes(const void *addr,size_t byteCount);

/**
 * Prints the given char
 *
 * @param c the character
 */
extern void debugChar(char c);

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
