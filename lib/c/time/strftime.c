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
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "timeintern.h"

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

const u8 daysPerMonth[2][12] = {
	/* DEF_YEAR */	{31,28,31,30,31,30,31,31,30,31,30,31},
	/* LEAP_YEAR */	{31,29,31,30,31,30,31,31,30,31,30,31}
};

size_t strftime(char *str,size_t max,const char *fmt,const struct tm *t) {
	size_t len;
	int wDay,number;
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
				if(t->tm_wday >= 0 && t->tm_wday < (int)ARRAY_SIZE(abrWeekDays))
					cpy = abrWeekDays[t->tm_wday];
				break;
			case 'A':
				if(t->tm_wday >= 0 && t->tm_wday < (int)ARRAY_SIZE(weekDays))
					cpy = weekDays[t->tm_wday];
				break;
			case 'b':
				if(t->tm_mon >= 0 && t->tm_mon < (int)ARRAY_SIZE(abrMonthNames))
					cpy = abrMonthNames[t->tm_mon];
				break;
			case 'B':
				if(t->tm_mon >= 0 && t->tm_mon < (int)ARRAY_SIZE(monthNames))
					cpy = monthNames[t->tm_mon];
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
				number = t->tm_mday + 1;
				break;
			case 'H':
				number = t->tm_hour;
				break;
			case 'I':
				if(t->tm_hour == 0)
					number = 12;
				else if(t->tm_hour > 12)
					number = t->tm_hour - 12;
				else
					number = t->tm_hour;
				break;
			case 'm':
				number = t->tm_mon + 1;
				break;
			case 'M':
				number = t->tm_min;
				break;
			case 'S':
				number = t->tm_sec;
				break;
			case 'U':
				/* TODO is this correct? */
				wDay = t->tm_wday;
				number = (t->tm_yday + 7 - wDay) / 7;
				break;
			case 'W':
				/* TODO is this correct? */
				wDay = t->tm_wday;
				number = (t->tm_yday + 7 - (wDay ? (wDay - 1) : 6)) / 7;
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
				if((len = strftime(str,end - str,cpy,t)) == 0)
					return 0;
				str += len;
				break;
			case 'j':
				if(snprintf(str,end - str,"%03d",t->tm_yday) < 3)
					return 0;
				str += 3;
				break;
			case 'p':
				if(snprintf(str,end - str,"%s",t->tm_hour < 12 ? "AM" : "PM") < 2)
					return 0;
				str += 2;
				break;
			case 'w':
				if(snprintf(str,end - str,"%d",t->tm_wday) < 1)
					return 0;
				str++;
				break;
			case 'y':
				/* do it here because 2010 is "encoded" as 110, i.e. 3 chars */
				if(snprintf(str,end - str,"%03d",t->tm_year) < 3)
					return 0;
				str += 3;
				break;
			case 'Y':
				if(snprintf(str,end - str,"%04d",t->tm_year + 1900) < 4)
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
