/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/date.h>
#include <esc/fileio.h>
#include <string.h>

/* timestamp stuff */
#define SECS_PER_MIN			60
#define SECS_PER_HOUR			(60 * SECS_PER_MIN)
#define SECS_PER_DAY			(24 * SECS_PER_HOUR)
#define SECS_PER_YEAR			(365 * SECS_PER_DAY)
#define SECS_PER_LEAPYEAR		(366 * SECS_PER_DAY)
#define IS_LEAP_YEAR(y)			(((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define DEF_YEAR				0
#define LEAP_YEAR				1

static u8 daysPerMonth[2][12] = {
	/* DEF_YEAR */ 	{31,28,31,30,31,30,31,31,30,31,30,31},
	/* LEAP_YEAR */	{31,29,31,30,31,30,31,31,30,31,30,31}
};

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

/*
 * 	%a	Abbreviated weekday name
 * 	%A	Full weekday name
 * 	%b	Abbreviated month name
 * 	%B	Full month name
 * 	%c	Date and time representation
 * 	%d	Day of the month (01-31)
 * 	%H	Hour in 24h format (00-23)
 * 	%I	Hour in 12h format (01-12)
 * 	%j	Day of the year (001-366)
 * 	%m	Month as a decimal number (01-12)
 * 	%M	Minute (00-59)
 * 	%p	AM or PM designation
 * 	%S	Second (00-61)
 * 	%U	Week number with the first Sunday as the first day of week one (00-53)
 * 	%w	Weekday as a decimal number with Sunday as 0 (0-6)
 * 	%W	Week number with the first Monday as the first day of week one (00-53)
 * 	%x	Date representation
 * 	%X	Time representation
 * 	%y	Year, last two digits (00-99)
 * 	%Y	Year
 * 	%Z	Timezone name or abbreviation
 * 	%%	A % sign
 */
s32 dateToString(char *str,u32 max,const char *fmt,sDate *date) {
	u32 len,wDay;
	char c;
	char *start = str;
	char *end = str + max - 1;
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
				cpy = abrWeekDays[date->weekDay - 1];
				break;
			case 'A':
				cpy = weekDays[date->weekDay - 1];
				break;
			case 'b':
				cpy = abrMonthNames[date->month - 1];
				break;
			case 'B':
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
		s16 number = -1;
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
			if(str + 2 >= end)
				return 0;
			sprintf(str,"%02d",number);
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
				if(str + 3 >= end)
					return 0;
				sprintf(str,"%03d",date->yearDay);
				str += 3;
				break;
			case 'p':
				if(str + 2 >= end)
					return 0;
				if(date->hour < 12)
					sprintf(str,"%s","AM");
				else
					sprintf(str,"%s","PM");
				str += 2;
				break;
			case 'w':
				if(str + 1 >= end)
					return 0;
				sprintf(str,"%d",date->weekDay - 1);
				str++;
				break;
			case 'Y':
				if(str + 4 >= end)
					return 0;
				sprintf(str,"%04d",date->year);
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

u32 getTime(void) {
	sDate date;
	if(getDate(&date) == 0)
		return getTimeOf(&date);
	return 0;
}

u32 getTimeOf(const sDate *date) {
	s32 m;
	u32 y,ts;
	u8 yearType;
	ts = 0;
	/* add full years */
	for(y = 1970; y < date->year; y++) {
		if(IS_LEAP_YEAR(y))
			ts += SECS_PER_LEAPYEAR;
		else
			ts += SECS_PER_YEAR;
	}
	/* add full months */
	yearType = IS_LEAP_YEAR(date->year) ? LEAP_YEAR : DEF_YEAR;
	for(m = (s32)date->month - 2; m >= 0; m--)
		ts += daysPerMonth[yearType][m] * SECS_PER_DAY;
	/* add full days */
	ts += (date->monthDay - 1) * SECS_PER_DAY;
	/* add hours, mins and secs */
	ts += date->hour * SECS_PER_HOUR;
	ts += date->min * SECS_PER_MIN;
	ts += date->sec;
	return ts;
}

s32 getDate(sDate *date) {
	s32 err;
	u8 yearType;
	s32 m;

	/* read date provided by CMOS */
	tFD fd = open("system:/bin/date",IO_READ);
	if(fd < 0)
		return fd;
	if((err = read(fd,date,sizeof(sDate))) < 0)
		return err;
	close(fd);

	/* set year-day */
	date->yearDay = date->monthDay - 1;
	yearType = IS_LEAP_YEAR(date->year) ? LEAP_YEAR : DEF_YEAR;
	for(m = (s32)date->month - 2; m >= 0; m--)
		date->yearDay += daysPerMonth[yearType][m];

	return 0;
}

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
	date->weekDay = (4 + days) % 7;
}
