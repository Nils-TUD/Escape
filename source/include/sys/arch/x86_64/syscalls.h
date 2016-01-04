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

#include <sys/common.h>
#include <sys/sysctrace.h>
#include <errno.h>

#define SYS_START(cnt,syscno,argc,...)							\
	if(traceFd != -1)											\
		syscTraceEnter(syscno,&cnt,argc,## __VA_ARGS__)

static inline long finish(uint32_t cnt,ulong res,long err) {
	errno = err;
	if(traceFd != -1)
		syscTraceLeave(cnt,res,err);
	if(EXPECT_FALSE(err))
		return err;
	return res;
}

static inline void syscalldbg(void) {
	__asm__ volatile ("int	$0x30\n" : : : "memory");
}

/* ignore that warning here, because the code is performance critical */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"

static inline long syscall0(long syscno) {
	ulong res, err;
	uint32_t cnt; /* does not need to be initialized */
	SYS_START(cnt,syscno,0);
	__asm__ volatile (
		"syscall"
		: "=a"(res), "=d"(err)
		: "D"(syscno)
		: "rcx", "r11", "r9", "memory"
	);
	return finish(cnt,res,err);
}

static inline long syscall1(long syscno,ulong arg1) {
	ulong res, err;
	uint32_t cnt; /* does not need to be initialized */
	SYS_START(cnt,syscno,1,arg1);
	__asm__ volatile (
		"syscall"
		: "=a"(res), "=d"(err)
		: "D"(syscno), "S"(arg1)
		: "rcx", "r11", "r9", "memory"
	);
	return finish(cnt,res,err);
}

static inline long syscall2(long syscno,ulong arg1,ulong arg2) {
	ulong res, err;
	uint32_t cnt; /* does not need to be initialized */
	SYS_START(cnt,syscno,2,arg1,arg2);
	register ulong r10 __asm__ ("r10") = arg2;
	__asm__ volatile (
		"syscall"
		: "=a"(res), "=d"(err)
		: "D"(syscno), "S"(arg1), "r"(r10)
		: "rcx", "r11", "r9", "memory"
	);
	return finish(cnt,res,err);
}

static inline long syscall3(long syscno,ulong arg1,ulong arg2,ulong arg3) {
	ulong res, err;
	uint32_t cnt; /* does not need to be initialized */
	SYS_START(cnt,syscno,3,arg1,arg2,arg3);
	register ulong r10 __asm__ ("r10") = arg2;
	register ulong r8 __asm__ ("r8") = arg3;
	__asm__ volatile (
		"syscall"
		: "=a"(res), "=d"(err)
		: "D"(syscno), "S"(arg1), "r"(r10), "r"(r8)
		: "rcx", "r11", "r9", "memory"
	);
	return finish(cnt,res,err);
}

static inline long syscall4(long syscno,ulong arg1,ulong arg2,ulong arg3,ulong arg4) {
	ulong res, err;
	uint32_t cnt; /* does not need to be initialized */
	SYS_START(cnt,syscno,4,arg1,arg2,arg3,arg4);
	register ulong r10 __asm__ ("r10") = arg2;
	register ulong r8 __asm__ ("r8") = arg3;
	__asm__ volatile (
		"push 	%6\n"
		"syscall\n"
		"add	$8,%%rsp\n"
		: "=a"(res), "=d"(err)
		: "D"(syscno), "S"(arg1), "r"(r10), "r"(r8), "r"(arg4)
		: "rcx", "r11", "r9", "memory"
	);
	return finish(cnt,res,err);
}

#pragma GCC diagnostic pop
