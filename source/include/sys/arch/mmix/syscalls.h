/**
 * $Id: syscalls.h 900 2011-06-02 20:18:17Z nasmussen $
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

#ifndef MMIX_SYSCALLS_H_
#define MMIX_SYSCALLS_H_

#include <esc/common.h>

/* some convenience-macros */
#define SYSC_SETERROR(stack,errorCode)	((stack)[1] = (errorCode))
#define SYSC_ERROR(stack,errorCode)		{ ((stack)[1] = (errorCode)); return; }
#define SYSC_RET1(stack,val)			((stack)[0] = (val))
#define SYSC_RET2(stack,val)			((stack)[1] = (val))
#define SYSC_NUMBER(stack)				((stack)[7])
#define SYSC_ARG1(stack)				((stack)[0])
#define SYSC_ARG2(stack)				((stack)[1])
#define SYSC_ARG3(stack)				((stack)[2])
#define SYSC_ARG4(stack)				((stack)[3])
#define SYSC_ARG5(stack)				((stack)[4])
#define SYSC_ARG6(stack)				((stack)[5])
#define SYSC_ARG7(stack)				((stack)[6])

#endif /* MMIX_SYSCALLS_H_ */
