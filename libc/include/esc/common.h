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

#ifndef COMMON_H_
#define COMMON_H_

#include <types.h>
#include <stddef.h>
#include <stdarg.h>

#ifndef NDEBUG
#define DEBUGGING 1
#endif

#define VTERM_COUNT		6

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The last error-code
 */
extern s32 errno;

/**
 * Displays an error-message according to given format and arguments and appends ': <errmsg>' if
 * errno is < 0. After that exit(EXIT_FAILURE) is called.
 *
 * @param fmt the error-message-format
 */
void error(const char *fmt,...) A_NORETURN;

/**
 * Calculates the stacktrace
 *
 * @return the trace (null-terminated)
 */
u32 *getStackTrace(void);

/**
 * Prints the stack-trace
 */
void printStackTrace(void);

#ifdef __cplusplus
}
#endif

#endif /*COMMON_H_*/
