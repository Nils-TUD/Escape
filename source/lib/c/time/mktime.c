/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <time.h>
#include "timeintern.h"

time_t mktime(struct tm *t) {
	int m,y,yearType;
	time_t ts = 0;
	/* add full years */
	for(y = 70; y < t->tm_year; y++) {
		if(IS_LEAP_YEAR(y + 1900))
			ts += SECS_PER_LEAPYEAR;
		else
			ts += SECS_PER_YEAR;
	}
	/* add full months */
	yearType = IS_LEAP_YEAR(t->tm_year + 1900) ? LEAP_YEAR : DEF_YEAR;
	for(m = t->tm_mon - 1; m >= 0; m--)
		ts += daysPerMonth[yearType][m] * SECS_PER_DAY;
	/* add full days */
	ts += t->tm_mday * SECS_PER_DAY;
	/* add hours, mins and secs */
	ts += t->tm_hour * SECS_PER_HOUR;
	ts += t->tm_min * SECS_PER_MIN;
	ts += t->tm_sec;
	return ts;
}
