/**
 * $Id$
 */

#ifndef TEST_SYSCALLS_H_
#define TEST_SYSCALLS_H_

#include <esc/common.h>

#ifdef __eco32__
#include "arch/eco32/syscalls.h"
#endif
#ifdef __i386__
#include "arch/i586/syscalls.h"
#endif
#ifdef __mmix__
#include "arch/mmix/syscalls.h"
#endif

int doSyscall7(ulong syscallNo,ulong arg1,ulong arg2,ulong arg3,ulong arg4,ulong arg5,
		ulong arg6,ulong arg7);
int doSyscall(ulong syscallNo,ulong arg1,ulong arg2,ulong arg3);

#endif /* TEST_SYSCALLS_H_ */
