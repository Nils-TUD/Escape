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

#ifndef RAND_H_
#define RAND_H_

#include <esc/common.h>

/* max rand-number */
#define RAND_MAX 0xFFFFFFFF

/**
 * Rand will generate a random number between 0 and 'RAND_MAX' (at least 32767).
 *
 * @return the random number
 */
s32 rand(void);

/**
 * Srand seeds the random number generation function rand so it does not produce the same
 * sequence of numbers.
 */
void srand(u32 seed);

#endif /* RAND_H_ */
