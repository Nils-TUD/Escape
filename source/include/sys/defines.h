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

/* exit-codes */
#define EXIT_FAILURE			1
#define EXIT_SUCCESS			0

#define ARRAY_SIZE(array)		(sizeof((array)) / sizeof((array)[0]))
/* static strlen() */
#define SSTRLEN(str)			(sizeof((str)) - 1)

#define MAX(a,b)				((a) > (b) ? (a) : (b))
#define MIN(a,b)				((a) > (b) ? (b) : (a))

#define EXPECT_FALSE(cond)		__builtin_expect(!!(cond), 0)
#define EXPECT_TRUE(cond)		__builtin_expect(!!(cond), 1)

/* gcc-attributes */
#define A_PACKED				__attribute__((packed))
#define A_ALIGNED(x)			__attribute__((aligned(x)))
#define A_CHECKRET				__attribute__((__warn_unused_result__))
#define A_NORETURN				__attribute__((noreturn))
#define A_INIT					__attribute__((section(".ctors")))
#define A_UNUSED				__attribute__((unused))
#define A_INLINE				__attribute__((inline))
#define A_NOINLINE				__attribute__((noinline))
#define A_ALWAYS_INLINE			__attribute__((always_inline))
#define A_UNREACHED				__builtin_unreachable()
#define A_REGPARM(x)			__attribute__((regparm(x)))
#define A_WEAK					__attribute__((weak))
#define A_NOASAN				__attribute__((no_sanitize_address))

#if defined(__cplusplus)
#	define EXTERN_C				extern "C"
#else
#	define EXTERN_C
#endif
