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

#ifndef I586_SYSCALLS_PROC_H_
#define I586_SYSCALLS_PROC_H_

#include <sys/intrpt.h>

/**
 * Requests some IO-ports
 *
 * @param uint16_t start-port
 * @param size_t number of ports
 * @return int 0 if successfull or a negative error-code
 */
int sysc_requestIOPorts(sIntrptStackFrame *stack);

/**
 * Releases some IO-ports
 *
 * @param uint16_t start-port
 * @param size_t number of ports
 * @return int 0 if successfull or a negative error-code
 */
int sysc_releaseIOPorts(sIntrptStackFrame *stack);

/**
 * Performs a VM86-interrupt. That means a VM86-task is created as a child-process, the
 * registers are set correspondingly and the tasks starts at the handler for the given interrupt.
 * As soon as the interrupt is finished the result is copied into the registers
 *
 * @param uint16_t the interrupt-number
 * @param sVM86Regs* the registers
 * @param sVM86Memarea* the memareas (may be NULL)
 * @param size_t mem-area count
 * @return int 0 on success
 */
int sysc_vm86int(sIntrptStackFrame *stack);

#endif /* I586_SYSCALLS_PROC_H_ */
