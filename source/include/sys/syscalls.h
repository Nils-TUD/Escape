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

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <sys/common.h>
#include <sys/intrpt.h>

#ifdef __i386__
#include <sys/arch/i586/syscalls.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/syscalls.h>
#endif

/**
 * Handles the syscall for the given stack
 *
 * @param intrptStack the pointer to the interrupt-stack
 */
void sysc_handle(sIntrptStackFrame *intrptStack);

/**
 * @param sysCallNo the syscall-number
 * @return the number of arguments for the given syscall
 */
uint sysc_getArgCount(uint sysCallNo);

/**
 * Checks whether the given null-terminated string (in user-space) is readable
 *
 * @param char* the string
 * @return true if so
 */
bool sysc_isStringReadable(const char *string);

#endif /* SYSCALLS_H_ */
