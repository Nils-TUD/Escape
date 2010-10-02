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

#ifndef DEFINES_H_
#define DEFINES_H_

#include <types.h>

/* exit-codes */
#define EXIT_FAILURE			1
#define EXIT_SUCCESS			0

#define ARRAY_SIZE(array)		(sizeof((array)) / sizeof((array)[0]))
/* static strlen() */
#define SSTRLEN(str)			(sizeof((str)) - 1)

#define MAX(a,b)				((a) > (b) ? (a) : (b))
#define MIN(a,b)				((a) > (b) ? (b) : (a))

/**
 * Assuming that <startx> < <endx> and <endx> is not included (that means with start=0 and end=10
 * 0 .. 9 is used), the macro determines whether the two ranges overlap anywhere.
 */
#define OVERLAPS(start1,end1,start2,end2) \
	(((start1) >= (start2) && (start1) < ((end2))) ||	/* start in range */	\
	((end1) > (start2) && (end1) <= (end2)) ||			/* end in range */		\
	((start1) < (start2) && (end1) > (end2)))			/* complete overlapped */

/* for declaring unused parameters */
#define UNUSED(x)				(void)(x)

/* gcc-attributes */
#define A_PACKED				__attribute__((packed))
#define A_ALIGNED(x)			__attribute__((aligned (x)))
#define A_CHECKRET				__attribute__((__warn_unused_result__))
#define A_NORETURN				__attribute__((noreturn))

/* process id */
typedef u16 tPid;
typedef u16 pid_t;
/* thread id */
typedef u16 tTid;
/* file-descriptor */
typedef s16 tFD;
/* inode-number */
typedef s32 tInodeNo;
/* device-number */
typedef s32 tDevNo;
/* signal-number */
typedef u8 tSig;
/* msg-id */
typedef u32 tMsgId;
typedef u32 tEvObj;

#ifdef __cplusplus
namespace std {
#endif

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef s32 ptrdiff_t;
#endif

#ifndef __SIZE_T__
#define __SIZE_T__
typedef u32 size_t;
#endif

typedef s32 ssize_t;
typedef s32 off_t;
typedef u16 mode_t;

#ifdef __cplusplus
}
#endif

#endif /* DEFINES_H_ */
