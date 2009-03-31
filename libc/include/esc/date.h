/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef DATE_H_
#define DATE_H_

#include <esc/common.h>

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
 * Copies into str the content of format, expanding its format tags into the corresponding
 * values as specified by date, with a limit of max characters.
 * You can use the following:
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
 *
 * @param str the string to fill
 * @param max the maximum number of chars to write to str
 * @param fmt the format
 * @param date the time-information
 * @return the number of written chars, if it fits, zero otherwise
 */
s32 dateToString(char *str,u32 max,const char *fmt,sDate *date);

/**
 * @return the current unix-timestamp
 */
u32 getTime(void);

/**
 * @param date the date
 * @return the unix-timestamp for the given date
 */
u32 getTimeOf(const sDate *date);

/**
 * Fills the given date-struct with the current date- and time information
 *
 * @param date the date-struct to fill
 * @return 0 on success
 */
s32 getDate(sDate *date);

/**
 * Stores the date- and time-information in the given date-structure from the given timestamp
 *
 * @param date the date-struct to fill
 * @param timestamp the timestamp to use
 */
void getDateOf(sDate *date,u32 timestamp);

#endif /* DATE_H_ */
