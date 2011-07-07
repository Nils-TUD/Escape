/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include "../../syscalls.h"

int doSyscall7(ulong syscallNo,ulong arg1,ulong arg2,ulong arg3,ulong arg4,ulong arg5,
		ulong arg6,ulong arg7) {
	int res;
	__asm__ __volatile__ (
		"movl	%2,%%ecx\n"
		"movl	%3,%%edx\n"
		"movl	%7,%%eax\n"
		"pushl	%%eax\n"
		"movl	%6,%%eax\n"
		"pushl	%%eax\n"
		"movl	%5,%%eax\n"
		"pushl	%%eax\n"
		"movl	%4,%%eax\n"
		"pushl	%%eax\n"
		"movl	%1,%%eax\n"
		"int	$0x30\n"
		"add	$16,%%esp\n"
		"test	%%ecx,%%ecx\n"
		"jz		1f\n"
		"movl	%%ecx,%%eax\n"
		"1:\n"
		"mov	%%eax,%0\n"
		: "=a" (res) : "m" (syscallNo), "m" (arg1), "m" (arg2), "m" (arg3), "m" (arg4),
		  "m" (arg5), "m" (arg6), "m" (arg7)
	);
	return res;
}

int doSyscall(ulong syscallNo,ulong arg1,ulong arg2,ulong arg3) {
	int res;
	__asm__ __volatile__ (
		"movl	%2,%%ecx\n"
		"movl	%3,%%edx\n"
		"movl	%4,%%eax\n"
		"pushl	%%eax\n"
		"movl	%1,%%eax\n"
		"int	$0x30\n"
		"add	$4,%%esp\n"
		"test	%%ecx,%%ecx\n"
		"jz		1f\n"
		"movl	%%ecx,%%eax\n"
		"1:\n"
		"mov	%%eax,%0\n"
		: "=a" (res) : "m" (syscallNo), "m" (arg1), "m" (arg2), "m" (arg3)
	);
	return res;
}
