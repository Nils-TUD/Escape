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

struct tm *gmtime(const time_t *timer) {
	static struct tm timeptr;
	time_t ts,timestamp = *timer;
	long nextSecs;
	int year,m,days,yearType;

	ts = timestamp;
	/* determine time */
	timeptr.tm_sec = timestamp % 60;
	timestamp /= 60;
	timeptr.tm_min = timestamp % 60;
	timestamp /= 60;
	timeptr.tm_hour = timestamp % 24;
	timestamp /= 24;

	/* determine year */
	days = timestamp;
	for(year = 1970; ; year++) {
		if(IS_LEAP_YEAR(year))
			nextSecs = (long)ts - SECS_PER_LEAPYEAR;
		else
			nextSecs = (long)ts - SECS_PER_YEAR;
		if(nextSecs < 0)
			break;
		ts = nextSecs;
	}

	/* now we have the number of seconds in the year */
	timeptr.tm_yday = ts / SECS_PER_DAY;
	yearType = IS_LEAP_YEAR(year) ? LEAP_YEAR : DEF_YEAR;
	for(m = 0; ; m++) {
		nextSecs = (long)ts - daysPerMonth[yearType][m] * SECS_PER_DAY;
		if(nextSecs < 0)
			break;
		ts = nextSecs;
	}
	timeptr.tm_mon = m;

	/* now we have the number of seconds in the month */
	timeptr.tm_mday = ts / SECS_PER_DAY;
	/* 1.1.1970 was thursday */
	timeptr.tm_wday = (4 + days) % 7;
	timeptr.tm_year = year - 1900;
	return &timeptr;
}
