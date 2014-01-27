/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#include <esc/common.h>

EXTERN_C long syscall0(long syscno);
EXTERN_C long syscall1(long syscno,ulong arg1);
EXTERN_C long syscall2(long syscno,ulong arg1,ulong arg2);
EXTERN_C long syscall3(long syscno,ulong arg1,ulong arg2,ulong arg3);
EXTERN_C long syscall4(long syscno,ulong arg1,ulong arg2,ulong arg3,ulong arg4);
EXTERN_C long syscall7(long syscno,ulong arg1,ulong arg2,ulong arg3,ulong arg4,ulong arg5,ulong arg6,
                       ulong arg7);

static inline void syscalldbg(void) {
	syscall0(SYSCALL_DEBUG);
}
