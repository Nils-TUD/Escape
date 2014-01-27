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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Determines the width of the given signed 32-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
size_t getnwidth(int n);

/**
 * Determines the width of the given signed 64-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
size_t getllwidth(llong n);

/**
 * Determines the width of the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
size_t getuwidth(uint n,uint base);

/**
 * Determines the width of the given unsigned 64-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
size_t getullwidth(ullong n,uint base);

#ifdef __cplusplus
}
#endif
