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

/* timestamp stuff */
 enum {
	SECS_PER_MIN			= 60,
	SECS_PER_HOUR			= 60 * SECS_PER_MIN,
	SECS_PER_DAY			= 24 * SECS_PER_HOUR,
	SECS_PER_YEAR			= 365 * SECS_PER_DAY,
	SECS_PER_LEAPYEAR		= 366 * SECS_PER_DAY,
};

enum {
	DEF_YEAR				= 0,
	LEAP_YEAR				= 1,
};

extern const uchar daysPerMonth[2][12];

static inline bool isLeapYear(int year) {
	return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}
