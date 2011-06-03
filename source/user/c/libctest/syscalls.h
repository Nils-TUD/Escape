/**
 * $Id$
 */

#ifndef TEST_SYSCALLS_H_
#define TEST_SYSCALLS_H_

#include <esc/common.h>

int doSyscall7(uint syscallNo,uint arg1,uint arg2,uint arg3,uint arg4,uint arg5,uint arg6,uint arg7);
int doSyscall(uint syscallNo,uint arg1,uint arg2,uint arg3);

#endif /* TEST_SYSCALLS_H_ */
