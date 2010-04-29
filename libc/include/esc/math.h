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

#ifndef MATH_H_
#define MATH_H_

#include <esc/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/* results of div and ldiv */
typedef struct {
	s32 quot;
	s32 rem;
} tDiv;
typedef struct {
	s64 quot;
	s64 rem;
} tLDiv;

/**
 * @param n the number
 * @return absolute value of <n>
 */
s32 abs(s32 n);

/**
 * @param n the number
 * @return absolute value of <n>
 */
s64 labs(s64 n);

/**
 * Returns the integral quotient and remainder of the division of numerator by denominator as a
 * structure of type tDiv, which has two members: quot and rem.
 *
 * @param numerator the numerator
 * @param denominator the denominator
 * @return quotient and remainder
 */
tDiv div(s32 numerator,s32 denominator);

/**
 * Returns the integral quotient and remainder of the division of numerator by denominator as a
 * structure of type tLDiv, which has two members: quot and rem.
 *
 * @param numerator the numerator
 * @param denominator the denominator
 * @return quotient and remainder
 */
tLDiv ldiv(s64 numerator,s64 denominator);

#ifdef __cplusplus
}
#endif

#endif /* MATH_H_ */
