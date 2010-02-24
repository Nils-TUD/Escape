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

#ifndef PRINTF_H_
#define PRINTF_H_

#include <common.h>
#include <stdarg.h>

typedef void (*fPrintc)(char c);
typedef void (*fEscape)(const char **fmt);
typedef u8 (*fPipePad)(void);

typedef struct {
	fPrintc print;		/* must be provided */
	fEscape escape;		/* optional, for handling escape-sequences */
	fPipePad pipePad;	/* optional, for returning the pad-value for '|' */
} sPrintEnv;

/**
 * The buffer for prf_(v)sprintf()
 */
typedef struct {
	u8 dynamic;
	char *str;
	u32 size;
	u32 len;
} sStringBuffer;

/**
 * Kernel-version of sprintf
 *
 * @param buf the buffer to use. if buf->dynamic is true, sprintf() manages the memory-allocation.
 * 	if buf->str is NULL it will be allocated at beginning, otherwise the existing buffer will
 *	be used.
 * @param fmt the format
 */
void prf_sprintf(sStringBuffer *buf,const char *fmt,...);

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
void prf_vsprintf(sStringBuffer *buf,const char *fmt,va_list ap);

/**
 * The general formatted printing routine for the kernel.
 *
 * @param env the environment with function-pointers printf uses
 * @param fmt the format
 */
void prf_printf(sPrintEnv *env,const char *fmt,...);

/**
 * Same as prf_printf, but with the va_list as argument
 *
 * @param env the environment with function-pointers printf uses
 * @param fmt the format
 * @param ap the argument-list
 */
void prf_vprintf(sPrintEnv *env,const char *fmt,va_list ap);

#endif /* PRINTF_H_ */
