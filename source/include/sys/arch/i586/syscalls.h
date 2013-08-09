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

#pragma once

#include <esc/common.h>

/* some convenience-macros */
#define SYSC_SETERROR(stack,errorCode)	((stack)->edi = (errorCode))
#define SYSC_ERROR(stack,errorCode)		{ ((stack)->edi = (errorCode)); return (errorCode); }
#define SYSC_RET1(stack,val)			{ ((stack)->eax = (val)); ((stack)->edi = 0); return 0; }
#define SYSC_NUMBER(stack)				((stack)->eax)
#define SYSC_ARG1(stack)				((stack)->esi)
#define SYSC_ARG2(stack)				((stack)->edi)
#define SYSC_ARG3(stack)				((stack)->ebx)
#define SYSC_ARG4(stack)				(*(((uint32_t*)(stack)->getSP()) + 0))
#define SYSC_ARG5(stack)				(*(((uint32_t*)(stack)->getSP()) + 1))
#define SYSC_ARG6(stack)				(*(((uint32_t*)(stack)->getSP()) + 2))
#define SYSC_ARG7(stack)				(*(((uint32_t*)(stack)->getSP()) + 3))
