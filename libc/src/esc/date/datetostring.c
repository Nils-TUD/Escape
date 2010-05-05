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
#include <stdio.h>
#include <string.h>

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

s32 dateToString(char *str,u32 max,const char *fmt,sDate *date) {
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
				if(date->weekDay > 0 && date->weekDay <= ARRAY_SIZE(abrWeekDays))
					cpy = abrWeekDays[date->weekDay - 1];
				break;
			case 'A':
				if(date->weekDay > 0 && date->weekDay <= ARRAY_SIZE(weekDays))
					cpy = weekDays[date->weekDay - 1];
				break;
			case 'b':
				if(date->month > 0 && date->month <= ARRAY_SIZE(abrMonthNames))
					cpy = abrMonthNames[date->month - 1];
				break;
			case 'B':
				if(date->month > 0 && date->month <= ARRAY_SIZE(monthNames))
					cpy = monthNames[date->month - 1];
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
				number = date->monthDay;
				break;
			case 'H':
				number = date->hour;
				break;
			case 'I':
				if(date->hour == 0)
					number = 12;
				else if(date->hour > 12)
					number = date->hour - 12;
				else
					number = date->hour;
				break;
			case 'm':
				number = date->month;
				break;
			case 'M':
				number = date->min;
				break;
			case 'S':
				number = date->sec;
				break;
			case 'y':
				number = date->year - 2000;
				break;
			case 'U':
				/* TODO is this correct? */
				wDay = date->weekDay - 1;
				number = (date->yearDay + 7 - wDay) / 7;
				break;
			case 'W':
				/* TODO is this correct? */
				wDay = date->weekDay - 1;
				number = (date->yearDay + 7 - (wDay ? (wDay - 1) : 6)) / 7;
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
				if((len = dateToString(str,end - str,cpy,date)) == 0)
					return 0;
				str += len;
				break;
			case 'j':
				if(snprintf(str,end - str,"%03d",date->yearDay) < 3)
					return 0;
				str += 3;
				break;
			case 'p':
				if(snprintf(str,end - str,"%s",date->hour < 12 ? "AM" : "PM") < 2)
					return 0;
				str += 2;
				break;
			case 'w':
				if(snprintf(str,end - str,"%d",date->weekDay - 1) < 1)
					return 0;
				str++;
				break;
			case 'Y':
				if(snprintf(str,end - str,"%04d",date->year) < 4)
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
