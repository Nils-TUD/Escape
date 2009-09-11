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

#ifndef ASPRINTF_H_
#define ASPRINTF_H_

#include <types.h>
#include <stddef.h>
#include <stdarg.h>

/**
 * The buffer for a(v)sprintf()
 */
typedef struct {
	u8 dynamic;
	char *str;
	u32 size;
	u32 len;
} sStringBuffer;

/**
 * Determines the required length for the given format and arguments
 *
 * @param fmt the format
 * @return the required length of the string util_sprintf() would write to
 */
u32 asprintfLen(const char *fmt,...);

/**
 * Determines the required length for the given format and arguments
 *
 * @param fmt the format
 * @param ap the argument-list
 * @return the required length of the string util_sprintf() would write to
 */
u32 avsprintfLen(const char *fmt,va_list ap);

/**
 * Kernel-version of sprintf
 *
 * @param buf the buffer to use. if buf->dynamic is true, sprintf() manages the memory-allocation.
 * 	if buf->str is NULL it will be allocated at beginning, otherwise the existing buffer will
 *	be used.
 * @param fmt the format
 * @return true on success (can just fail if buf->dynamic is true)
 */
bool asprintf(sStringBuffer *buf,const char *fmt,...);

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
bool avsprintf(sStringBuffer *buf,const char *fmt,va_list ap);


#endif /* ASPRINTF_H_ */
