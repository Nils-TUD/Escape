/**
 * @version		$Id: syscalls.h 77 2008-11-22 22:27:35Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include "common.h"
#include "intrpt.h"

/* the user-stack within a syscall */
typedef struct {
	s32 number;	/* = error-code */
	u32 arg1;	/* = ret-val 1 */
	u32 arg2;	/* = ret-val 2 */
	u32 arg3;
	u32 arg4;
} sSysCallStack;

/**
 * Handles the syscall for the given stack
 *
 * @param intrptStack the pointer to the interrupt-stack
 */
void sysc_handle(sIntrptStackFrame *intrptStack);

#endif /* SYSCALLS_H_ */
