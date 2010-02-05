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

#include <esc/common.h>
#include <esc/date.h>
#include "dateintern.h"

const u8 daysPerMonth[2][12] = {
	/* DEF_YEAR */ 	{31,28,31,30,31,30,31,31,30,31,30,31},
	/* LEAP_YEAR */	{31,29,31,30,31,30,31,31,30,31,30,31}
};

void getDateOf(sDate *date,u32 timestamp) {
	u32 m,days,ts;
	s32 nextSecs;
	u8 yearType;

	ts = timestamp;
	/* determine time */
	date->sec = timestamp % 60;
	timestamp /= 60;
	date->min = timestamp % 60;
	timestamp /= 60;
	date->hour = timestamp % 24;
	timestamp /= 24;

	/* determine year */
	days = timestamp;
	for(date->year = 1970; ; date->year++) {
		if(IS_LEAP_YEAR(date->year))
			nextSecs = ts - SECS_PER_LEAPYEAR;
		else
			nextSecs = ts - SECS_PER_YEAR;
		if(nextSecs < 0)
			break;
		ts = nextSecs;
	}

	/* now we have the number of seconds in the year */
	date->yearDay = ts / SECS_PER_DAY;
	yearType = IS_LEAP_YEAR(date->year) ? LEAP_YEAR : DEF_YEAR;
	for(m = 0; ; m++) {
		nextSecs = ts - daysPerMonth[yearType][m] * SECS_PER_DAY;
		if(nextSecs < 0)
			break;
		ts = nextSecs;
	}
	date->month = m + 1;

	/* now we have the number of seconds in the month */
	date->monthDay = 1 + (ts / SECS_PER_DAY);
	/* TODO */
	date->isDst = 0;
	/* 1.1.1970 was thursday */
	date->weekDay = ((4 + days) % 7) + 1;
}
