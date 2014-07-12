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

#include <sys/common.h>

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

#if defined(__cplusplus)
extern "C" {
#endif

int pow(int a,int b);
double sqrt(double x);

double sin(double x);
double cos(double x);
double tan(double x);

int isfinite(double x);
int isfinitef(float x);

int isnan(double x);
int isnanf(float x);

int isinf(double x);
int isinff(float x);

double round(double x);
float roundf(float x);

#if defined(__cplusplus)
}
#endif
