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
