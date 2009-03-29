/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/io.h"
#include "../h/time.h"

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

u32 getTime(void) {
	sDate date;
	if(getDate(&date) == 0) {
		s32 m;
		u32 y,ts;
		u8 yearType;
		ts = 0;
		/* add full years */
		for(y = 1970; y < date.year; y++) {
			if(IS_LEAP_YEAR(y))
				ts += SECS_PER_LEAPYEAR;
			else
				ts += SECS_PER_YEAR;
		}
		/* add full months */
		yearType = IS_LEAP_YEAR(date.year) ? LEAP_YEAR : DEF_YEAR;
		for(m = (s32)date.month - 2; m >= 0; m--)
			ts += daysPerMonth[yearType][m] * SECS_PER_DAY;
		/* add full days */
		ts += (date.monthDay - 1) * SECS_PER_DAY;
		/* add hours, mins and secs */
		ts += date.hour * SECS_PER_HOUR;
		ts += date.min * SECS_PER_MIN;
		ts += date.sec;
		return ts;
	}
	return 0;
}

s32 getDate(sDate *date) {
	s32 err;
	tFD fd = open("system:/date",IO_READ);
	if(fd < 0)
		return fd;
	if((err = read(fd,date,sizeof(sDate))) < 0)
		return err;
	close(fd);
	return 0;
}
