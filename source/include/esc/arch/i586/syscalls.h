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

#include <esc/common.h>
#include <errno.h>

#ifdef SHAREDLIB
#	define DO_SYSENTER				\
	"call	1f\n"					\
	"1:\n"							\
	"pop	%%edx\n"				\
	"add	$(1f - 1b),%%edx\n"		\
	"mov	%%esp,%%ecx\n"			\
	"sysenter\n"					\
	"1:\n"
#else
#	define DO_SYSENTER				\
	"mov	$1f,%%edx\n"			\
	"mov	%%esp,%%ecx\n"			\
	"sysenter\n"					\
	"1:\n"
#endif

static inline long finish(uint32_t res,long err) {
	errno = err;
	if(EXPECT_FALSE(err))
		return err;
	return res;
}

static inline void syscalldbg(void) {
	__asm__ volatile ("int	$0x30\n" : : : "memory");
}

static inline long syscall0(long syscno) {
	ulong dummy, err;
	__asm__ volatile (
		DO_SYSENTER
		: "+a"(syscno), "=c"(dummy), "=d"(dummy), "=D"(err)
		:
		: "memory"
	);
	return finish(syscno,err);
}

static inline long syscall1(long syscno,ulong arg1) {
	ulong dummy, err;
	__asm__ volatile (
		DO_SYSENTER
		: "+a"(syscno), "=c"(dummy), "=d"(dummy), "=D"(err)
		: "S"(arg1)
		: "memory"
	);
	return finish(syscno,err);
}

static inline long syscall2(long syscno,ulong arg1,ulong arg2) {
	ulong dummy;
	__asm__ volatile (
		DO_SYSENTER
		: "+a"(syscno), "=c"(dummy), "=d"(dummy), "+D"(arg2)
		: "S"(arg1)
		: "memory"
	);
	return finish(syscno,arg2);
}

static inline long syscall3(long syscno,ulong arg1,ulong arg2,ulong arg3) {
	ulong dummy;
	__asm__ volatile (
		"push	%5\n"
		DO_SYSENTER
		"add	$4,%%esp\n"
		: "+a"(syscno), "=c"(dummy), "=d"(dummy), "+D"(arg2)
		: "S"(arg1), "g"(arg3)
		: "memory"
	);
	return finish(syscno,arg2);
}

static inline long syscall4(long syscno,ulong arg1,ulong arg2,ulong arg3,ulong arg4) {
	__asm__ volatile (
		// the problem is here that we need to push 2 values. thus, the second one can't be loaded
		// over esp, since the first push changes that. Using "r" doesn't work either because
		// 1. we have to tell him that ecx and edx are changed in the asm and 2. when doing that
		// he doesn't have enough registers. As it seems, using "+c" and "+d" does the right thing.
		// although it is suboptimal because the value might be an immediate which will then first
		// be put in e.g. ecx and than pushed.
		"push	%2\n"
		"push	%1\n"
		DO_SYSENTER
		"add	$8,%%esp\n"
		: "+a"(syscno), "+c"(arg3), "+d"(arg4), "+D"(arg2)
		: "S"(arg1)
		: "memory"
	);
	return finish(syscno,arg2);
}

EXTERN_C long syscall7(long syscno,ulong arg1,ulong arg2,ulong arg3,ulong arg4,ulong arg5,ulong arg6,
                       ulong arg7);
