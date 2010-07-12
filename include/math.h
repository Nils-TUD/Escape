/**
 * $Id: math.h 578 2010-03-29 15:54:22Z nasmussen $
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

#ifndef MATH_H_
#define MATH_H_

#include <esc/common.h>

typedef float float_t;
typedef double double_t;

#define HUGE_VAL	((double)0x7ff0000000000000ULL)
#define HUGE_VALF	((float)0x7f800000)
#define HUGE_VALL	HUGE_VAL

#define INFINITY	HUGE_VALF
#define NAN			((float)0x7FFFFFFF)

#define FP_ILOGB0	(-2147483647 - 1)
#define FP_ILOGBNAN	(-2147483647 - 1)

enum { FP_NAN, FP_INFINITE, FP_ZERO, FP_SUBNORMAL, FP_NORMAL };

s32 pow(s32 a,s32 b);

#endif /* MATH_H_ */
