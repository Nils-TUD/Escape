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

#include <sys/intrpt.h>

#ifdef __i386__
#include <sys/arch/i586/syscalls/proc.h>
#endif

/**
 * Returns the pid of the current process
 *
 * @return pid_t the pid
 */
int sysc_getpid(sIntrptStackFrame *stack);

/**
 * Returns the parent-pid of the given process
 *
 * @param pid the process-id
 * @return pid_t the parent-pid
 */
int sysc_getppid(sIntrptStackFrame *stack);

/**
 * Clones the current process
 *
 * @return pid_t 0 for the child, the child-pid for the parent-process
 */
int sysc_fork(sIntrptStackFrame *stack);

/**
 * Waits until a child has terminated
 *
 * @param sExitState* will be filled with information about the terminated process
 * @return int 0 on success
 */
int sysc_waitChild(sIntrptStackFrame *stack);

/**
 * Exchanges the process-data with another program
 *
 * @param char* the program-path
 */
int sysc_exec(sIntrptStackFrame *stack);

/**
 * Returns the environment-variable-name with index i. Or an error if there is none.
 *
 * @param char* the buffer to write the name to
 * @param size_t the size of the buffer
 * @param size_t the index of the variable
 * @return ssize_t the length of the name on success
 */
int sysc_getenvito(sIntrptStackFrame *stack);

/**
 * Returns the value of the environment-variable with given name
 *
 * @param char* the buffer to write the name to
 * @param size_t the size of the buffer
 * @param const char* the name
 * @return ssize_t the length of the value on success
 */
int sysc_getenvto(sIntrptStackFrame *stack);

/**
 * Sets the environment-variable with given name to given value
 *
 * @param const char* the name
 * @param const char* the value
 * @return int 0 on success
 */
int sysc_setenv(sIntrptStackFrame *stack);

#endif /* SYSCALLS_PROC_H_ */
