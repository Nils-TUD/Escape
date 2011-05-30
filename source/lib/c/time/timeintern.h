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

#ifndef TIMEINTERN_H_
#define TIMEINTERN_H_

#include <esc/common.h>
#include <time.h>

/* timestamp stuff */
#define SECS_PER_MIN			60
#define SECS_PER_HOUR			(60 * SECS_PER_MIN)
#define SECS_PER_DAY			(24 * SECS_PER_HOUR)
#define SECS_PER_YEAR			(365 * SECS_PER_DAY)
#define SECS_PER_LEAPYEAR		(366 * SECS_PER_DAY)
#define IS_LEAP_YEAR(y)			(((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define DEF_YEAR				0
#define LEAP_YEAR				1

extern const uchar daysPerMonth[2][12];

int readdate(struct tm *t);

#endif /* TIMEINTERN_H_ */
