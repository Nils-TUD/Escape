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
