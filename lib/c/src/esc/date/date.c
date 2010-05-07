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
#include <esc/exceptions/date.h>
#include <esc/mem/heap.h>
#include <esc/date.h>
#include <string.h>
#include <stdio.h>

/* timestamp stuff */
#define SECS_PER_MIN			60
#define SECS_PER_HOUR			(60 * SECS_PER_MIN)
#define SECS_PER_DAY			(24 * SECS_PER_HOUR)
#define SECS_PER_YEAR			(365 * SECS_PER_DAY)
#define SECS_PER_LEAPYEAR		(366 * SECS_PER_DAY)
#define IS_LEAP_YEAR(y)			(((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define DEF_YEAR				0
#define LEAP_YEAR				1

static void date_init(sDate *d);
static void date_set(sDate *d,tTime timestamp);
static tTime date_getTS(u8 month,u8 day,u16 year,u8 hour,u8 min,u8 sec);
static s32 date_format(sDate *d,char *str,u32 max,const char *fmt);

static const char *weekDays[] = {
	"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};
static const char *abrWeekDays[] = {
	"Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};
static const char *monthNames[] = {
	"January","February","March","April","May","June","July","August","September","October",
	"November","December"
};
static const char *abrMonthNames[] = {
	"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

static const u8 daysPerMonth[2][12];

sDate date_get(void) {
	sDate d;
	sCMOSDate cd;
	if(getDate(&cd) < 0)
		THROW(DateException);
	*(u8*)&d.hour = cd.hour;
	*(u8*)&d.min = cd.min;
	*(u8*)&d.sec = cd.sec;
	*(u8*)&d.month = cd.month;
	*(u8*)&d.monthDay = cd.monthDay;
	*(u16*)&d.year = cd.year;
	*(u8*)&d.weekDay = cd.weekDay;
	*(u16*)&d.yearDay = cd.yearDay;
	*(tTime*)&d.timestamp = cd.timestamp;
	date_init(&d);
	return d;
}

sDate date_getOfTS(tTime ts) {
	sDate d;
	date_set(&d,ts);
	date_init(&d);
	return d;
}

sDate date_getOfDate(u8 month,u8 day,u16 year) {
	sDate d;
	tTime ts = date_getTS(month,day,year,0,0,0);
	date_set(&d,ts);
	date_init(&d);
	return d;
}

sDate date_getOfTime(u8 hour,u8 min,u8 sec) {
	sDate d;
	sCMOSDate cd;
	tTime ts;
	if(getDate(&cd) < 0)
		THROW(DateException);
	ts = date_getTS(cd.month,cd.monthDay,cd.year,hour,min,sec);
	date_set(&d,ts);
	date_init(&d);
	return d;
}

sDate date_getOfDateTime(u8 month,u8 day,u16 year,u8 hour,u8 min,u8 sec) {
	sDate d;
	tTime ts = date_getTS(month,day,year,hour,min,sec);
	date_set(&d,ts);
	date_init(&d);
	return d;
}

static void date_init(sDate *d) {
	d->format = date_format;
	d->set = date_set;
}

static void date_set(sDate *d,tTime timestamp) {
	u32 m,days,ts,year;
	s32 nextSecs;
	u8 yearType;

	*(tTime*)&d->timestamp = timestamp;

	ts = timestamp;
	/* determine time */
	*(u8*)&d->sec = timestamp % 60;
	timestamp /= 60;
	*(u8*)&d->min = timestamp % 60;
	timestamp /= 60;
	*(u8*)&d->hour = timestamp % 24;
	timestamp /= 24;

	/* determine year */
	days = timestamp;
	for(year = 1970; ; year++) {
		if(IS_LEAP_YEAR(year))
			nextSecs = ts - SECS_PER_LEAPYEAR;
		else
			nextSecs = ts - SECS_PER_YEAR;
		if(nextSecs < 0)
			break;
		ts = nextSecs;
	}

	/* now we have the number of seconds in the year */
	*(u16*)&d->yearDay = ts / SECS_PER_DAY;
	yearType = IS_LEAP_YEAR(year) ? LEAP_YEAR : DEF_YEAR;
	for(m = 0; ; m++) {
		nextSecs = ts - daysPerMonth[yearType][m] * SECS_PER_DAY;
		if(nextSecs < 0)
			break;
		ts = nextSecs;
	}
	*(u8*)&d->month = m + 1;

	/* now we have the number of seconds in the month */
	*(u8*)&d->monthDay = 1 + (ts / SECS_PER_DAY);
	/* TODO */
	*(u8*)&d->isDst = -1;
	/* 1.1.1970 was thursday */
	*(u8*)&d->weekDay = ((4 + days) % 7) + 1;
	*(u16*)&d->year = year;
}

static tTime date_getTS(u8 month,u8 day,u16 year,u8 hour,u8 min,u8 sec) {
	s32 m;
	u32 y;
	u8 yearType;
	tTime ts = 0;
	/* add full years */
	for(y = 1970; y < year; y++) {
		if(IS_LEAP_YEAR(y))
			ts += SECS_PER_LEAPYEAR;
		else
			ts += SECS_PER_YEAR;
	}
	/* add full months */
	yearType = IS_LEAP_YEAR(year) ? LEAP_YEAR : DEF_YEAR;
	for(m = (s32)month - 2; m >= 0; m--)
		ts += daysPerMonth[yearType][m] * SECS_PER_DAY;
	/* add full days */
	ts += (day - 1) * SECS_PER_DAY;
	/* add hours, mins and secs */
	ts += hour * SECS_PER_HOUR;
	ts += min * SECS_PER_MIN;
	ts += sec;
	return ts;
}

static s32 date_format(sDate *d,char *str,u32 max,const char *fmt) {
	u32 len,wDay;
	s32 number;
	char c;
	char *start = str;
	char *end = str + max;
	const char *cpy;
	/* we want to terminate the string */
	max--;

	while(1) {
		while((c = *fmt++) != '%') {
			if(c == '\0') {
				*str = '\0';
				return str - start;
			}
			if(str >= end)
				return 0;
			*str++ = c;
		}

		c = *fmt++;

		/* string */
		cpy = NULL;
		switch(c) {
			/* TODO all 4 should be locale-dependend */
			case 'a':
				if(d->weekDay > 0 && d->weekDay <= ARRAY_SIZE(abrWeekDays))
					cpy = abrWeekDays[d->weekDay - 1];
				break;
			case 'A':
				if(d->weekDay > 0 && d->weekDay <= ARRAY_SIZE(weekDays))
					cpy = weekDays[d->weekDay - 1];
				break;
			case 'b':
				if(d->month > 0 && d->month <= ARRAY_SIZE(abrMonthNames))
					cpy = abrMonthNames[d->month - 1];
				break;
			case 'B':
				if(d->month > 0 && d->month <= ARRAY_SIZE(monthNames))
					cpy = monthNames[d->month - 1];
				break;
		}
		if(cpy != NULL) {
			len = strlen(cpy);
			if(str + len >= end)
				return 0;
			strcpy(str,cpy);
			str += len;
			continue;
		}

		/* 2 digit number */
		number = -1;
		switch(c) {
			case 'd':
				number = d->monthDay;
				break;
			case 'H':
				number = d->hour;
				break;
			case 'I':
				if(d->hour == 0)
					number = 12;
				else if(d->hour > 12)
					number = d->hour - 12;
				else
					number = d->hour;
				break;
			case 'm':
				number = d->month;
				break;
			case 'M':
				number = d->min;
				break;
			case 'S':
				number = d->sec;
				break;
			case 'y':
				number = d->year - 2000;
				break;
			case 'U':
				/* TODO is this correct? */
				wDay = d->weekDay - 1;
				number = (d->yearDay + 7 - wDay) / 7;
				break;
			case 'W':
				/* TODO is this correct? */
				wDay = d->weekDay - 1;
				number = (d->yearDay + 7 - (wDay ? (wDay - 1) : 6)) / 7;
				break;
		}
		if(number != -1) {
			if(snprintf(str,end - str,"%02d",number) < 2)
				return 0;
			str += 2;
			continue;
		}

		/* other */
		switch(c) {
			/* TODO should be locale-dependend */
			case 'x':
			case 'X':
			case 'c':
				if(c == 'c')
					cpy = "%a %b %d %H:%M:%S %Y";
				else if(c == 'x')
					cpy = "%m/%d/%y";
				else
					cpy = "%H:%M:%S";
				if((len = d->format(d,str,end - str,cpy)) == 0)
					return 0;
				str += len;
				break;
			case 'j':
				if(snprintf(str,end - str,"%03d",d->yearDay) < 3)
					return 0;
				str += 3;
				break;
			case 'p':
				if(snprintf(str,end - str,"%s",d->hour < 12 ? "AM" : "PM") < 2)
					return 0;
				str += 2;
				break;
			case 'w':
				if(snprintf(str,end - str,"%d",d->weekDay - 1) < 1)
					return 0;
				str++;
				break;
			case 'Y':
				if(snprintf(str,end - str,"%04d",d->year) < 4)
					return 0;
				str += 4;
				break;
			case 'Z':
				/* TODO */
				break;
			default:
				if(str + 1 >= end)
					return 0;
				*str++ = c;
				break;
		}
	}
}
