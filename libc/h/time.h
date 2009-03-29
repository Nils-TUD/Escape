/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef TIME_H_
#define TIME_H_

#include "common.h"

/* all information about the current date and time */
typedef struct {
	u8 sec;
	u8 min;
	u8 hour;
	/* 1 - 7; 1 = sunday */
	u8 weekDay;
	/* 1 - 31 */
	u8 monthDay;
	/* 1 - 12 */
	u8 month;
	/* full year */
	u16 year;
	/* 1 - 365 */
	u16 yearDay;
	/* wether daylightsaving time is enabled */
	u8 isDst;
} sDate;

/**
 * @return the unix-timestamp
 */
u32 getTime(void);

/**
 * Fills the given date-struct with the current date- and time information
 *
 * @param date the date-struct to fill
 * @return 0 on success
 */
s32 getDate(sDate *date);

#endif /* TIME_H_ */
