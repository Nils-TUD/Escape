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

#ifndef SYSCALLS_PROC_H_
#define SYSCALLS_PROC_H_

#include <machine/intrpt.h>

/**
 * Returns the pid of the current process
 *
 * @return tPid the pid
 */
void sysc_getpid(sIntrptStackFrame *stack);

/**
 * Returns the parent-pid of the given process
 *
 * @param pid the process-id
 * @return tPid the parent-pid
 */
void sysc_getppid(sIntrptStackFrame *stack);

/**
 * Clones the current process
 *
 * @return tPid 0 for the child, the child-pid for the parent-process
 */
void sysc_fork(sIntrptStackFrame *stack);

/**
 * Destroys the process and issues a context-switch
 *
 * @param u32 the exit-code
 */
void sysc_exit(sIntrptStackFrame *stack);

/**
 * Blocks the process until an event occurrs
 */
void sysc_wait(sIntrptStackFrame *stack);

/**
 * Waits until a child has terminated
 *
 * @param sExitState* will be filled with information about the terminated process
 * @return s32 o on success
 */
void sysc_waitChild(sIntrptStackFrame *stack);

/**
 * Blocks the process for a given number of milliseconds
 *
 * @param u32 the number of msecs
 * @return s32 0 on success or a negative error-code
 */
void sysc_sleep(sIntrptStackFrame *stack);

/**
 * Releases the CPU (reschedule)
 */
void sysc_yield(sIntrptStackFrame *stack);

/**
 * Exchanges the process-data with another program
 *
 * @param char* the program-path
 */
void sysc_exec(sIntrptStackFrame *stack);

/**
 * Requests some IO-ports
 *
 * @param u16 start-port
 * @param u16 number of ports
 * @return s32 0 if successfull or a negative error-code
 */
void sysc_requestIOPorts(sIntrptStackFrame *stack);

/**
 * Releases some IO-ports
 *
 * @param u16 start-port
 * @param u16 number of ports
 * @return s32 0 if successfull or a negative error-code
 */
void sysc_releaseIOPorts(sIntrptStackFrame *stack);

/**
 * Performs a VM86-interrupt. That means a VM86-task is created as a child-process, the
 * registers are set correspondingly and the tasks starts at the handler for the given interrupt.
 * As soon as the interrupt is finished the result is copied into the registers
 *
 * @param u16 the interrupt-number
 * @param sVM86Regs* the registers
 * @param sVM86Memarea* the memareas (may be NULL)
 * @param u16 mem-area count
 * @return 0 on success
 */
void sysc_vm86int(sIntrptStackFrame *stack);

/**
 * Returns the cpu-cycles for the current thread
 *
 * @return u64 the cpu-cycles
 */
void sysc_getCycles(sIntrptStackFrame *stack);

#endif /* SYSCALLS_PROC_H_ */
