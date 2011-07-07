/**
 * $Id$
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
