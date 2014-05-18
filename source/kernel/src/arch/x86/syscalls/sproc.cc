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

#include <sys/common.h>
#include <sys/arch/x86/task/ioports.h>
#include <sys/task/proc.h>
#include <sys/syscalls.h>
#include <errno.h>

int Syscalls::reqports(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	uint16_t start = SYSC_ARG1(stack);
	size_t count = SYSC_ARG2(stack);

	/* check range */
	if(count == 0 || count > 0xFFFF || (uint32_t)start + count > 0xFFFF)
		SYSC_ERROR(stack,-EINVAL);

	int err = IOPorts::request(start,count);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}

int Syscalls::relports(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	uint16_t start = SYSC_ARG1(stack);
	size_t count = SYSC_ARG2(stack);

	/* check range */
	if(count == 0 || count > 0xFFFF || (uint32_t)start + count > 0xFFFF)
		SYSC_ERROR(stack,-EINVAL);

	int err = IOPorts::release(start,count);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}
