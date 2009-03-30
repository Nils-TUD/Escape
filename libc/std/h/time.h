/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef TIME_H_
#define TIME_H_

#include <types.h>

/* timestamp */
typedef u32 time_t;

/* time-struct */
struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

/**
 * Calculates the difference in seconds between time1 and time2.
 *
 * @param time2 the end-time
 * @param time1 the start-time
 * @return the difference in seconds
 */
double difftime(time_t time2,time_t time1);

/**
 * Creates a timestamp for the given time-information
 *
 * @param timeptr the time-information
 * @return the timestamp
 */
time_t mktime(struct tm *timeptr);

/**
 * Get the current calendar time as a time_t object.
 * The function returns this value, and if the argument is not a null pointer, the value is also
 * set to the object pointed by timer.
 *
 * @param timer will be set if != NULL
 * @return the timestamp
 */
time_t time(time_t *timer);

/**
 * Convert tm structure to string. Interprets the contents of the tm structure pointed by timeptr
 * as a calendar time and converts it to a C string containing a human-readable version
 * of the corresponding date and time.
 * The returned string has the following format:
 * <weekDay> <month> <day> hh:mm:ss yyyy\n\0
 *
 * @param timeptr the time-information
 * @return the string
 */
char *asctime(const struct tm *timeptr);

/**
 * This function is equivalent to: asctime(localtime(timer)).
 *
 * @param timer the timestamp
 * @return the string
 */
char *ctime(const time_t *timer);

/**
 * Uses the value pointed by timer to fill a tm structure with the values that represent the
 * corresponding time, expressed as UTC (or GMT timezone).
 *
 * @param timer the pointer to the timestamp
 * @return a pointer to a tm structure with the time information filled in (statically allocated)
 */
struct tm *gmtime(const time_t *timer);

/**
 * Uses the time pointed by timer to fill a tm structure with the values that represent the
 * corresponding local time.
 *
 * @param timer the pointer to the timestamp
 * @return a pointer to a tm structure with the time information filled in (statically allocated)
 */
struct tm *localtime(const time_t *timer);

/**
 * Copies into ptr the content of format, expanding its format tags into the corresponding
 * values as specified by timeptr, with a limit of maxsize characters.
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
 * @param ptr the string to fill
 * @param maxsize the maximum number of chars to write to ptr
 * @param format the format
 * @param timeptr the time-information
 * @return the number of written chars, if it fits, zero otherwise
 */
size_t strftime(char *ptr,size_t maxsize,const char *format,const struct tm *timeptr);

#endif /* TIME_H_ */
