/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include "common.h"
#include "intrpt.h"

/**
 * Handles the syscall for the given interrupt-stack
 *
 * @param stack the pointer to the interrupt-stack
 */
void sysc_handle(tIntrptStackFrame *stack);

#endif /* SYSCALLS_H_ */
