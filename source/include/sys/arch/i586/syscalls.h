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

#ifndef I586_SYSCALLS_H_
#define I586_SYSCALLS_H_

#include <esc/common.h>

/* some convenience-macros */
#define SYSC_SETERROR(stack,errorCode)	((stack)->ebx = (errorCode))
#define SYSC_ERROR(stack,errorCode)		{ ((stack)->ebx = (errorCode)); return (errorCode); }
#define SYSC_RET1(stack,val)			{ ((stack)->eax = (val)); return 0; }
#define SYSC_SETRET2(stack,val)			((stack)->edx = (val))
#define SYSC_NUMBER(stack)				((stack)->eax)
#define SYSC_ARG1(stack)				((stack)->ecx)
#define SYSC_ARG2(stack)				((stack)->edx)
#define SYSC_ARG3(stack)				(*((uint32_t*)(stack)->uesp))
#define SYSC_ARG4(stack)				(*(((uint32_t*)(stack)->uesp) + 1))
#define SYSC_ARG5(stack)				(*(((uint32_t*)(stack)->uesp) + 2))
#define SYSC_ARG6(stack)				(*(((uint32_t*)(stack)->uesp) + 3))
#define SYSC_ARG7(stack)				(*(((uint32_t*)(stack)->uesp) + 4))

#endif /* I586_SYSCALLS_H_ */
