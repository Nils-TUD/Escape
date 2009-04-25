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

#ifndef STDDEF_H_
#define STDDEF_H_

#include <types.h>

#define offsetof(type,field)	((size_t)(&((type *)0)->field))

#ifdef __cplusplus
#	define NULL					0
#else
#	define NULL					(void*)0
#endif

#define K						1024
#define M						(1024 * K)
#define G						(1024 * M)

#define ARRAY_SIZE(array)		(sizeof((array)) / sizeof((array)[0]))

#define MAX(a,b)				((a) > (b) ? (a) : (b))
#define MIN(a,b)				((a) > (b) ? (b) : (a))

/* for declaring unused parameters */
#define UNUSED(x)				(void)(x)

/* process id */
typedef u16 tPid;
/* VFS node number */
typedef u32 tVFSNodeNo;
/* file-descriptor */
typedef s16 tFD;
/* inode-number */
typedef s32 tInodeNo;
/* signal-number */
typedef u8 tSig;
/* service-id */
typedef s32 tServ;

typedef u32 ptrdiff_t;
typedef u32 size_t;

#endif /* STDDEF_H_ */
