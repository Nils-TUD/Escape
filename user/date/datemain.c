/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <date.h>
#include <bufio.h>

static const char *weekDays[] = {
	"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

int main(void) {
	u32 ts;
	sDate date;
	char str[1000];

	ts = getTime();
	getDate(&date);

	printf("UNIX-Timestamp: %d\n",ts);
	printf("%s, %02d/%02d/%04d, %02d:%02d:%02d, %d day of year\n",weekDays[date.weekDay],
			date.month,date.monthDay,date.year,date.hour,date.min,date.sec,date.yearDay);

	getDateOf(&date,ts);
	printf("%s, %02d/%02d/%04d, %02d:%02d:%02d, %d day of year\n",weekDays[date.weekDay],
			date.month,date.monthDay,date.year,date.hour,date.min,date.sec,date.yearDay);

	dateToString(str,1000,"a='%a', A='%A', b='%b', B='%B', c='%c', d='%d', H='%H', "
			"I='%I', j='%j', m='%m', M='%M', p='%p', S='%S', w='%w', x='%x', X='%X', "
			"y='%y', Y='%Y', U='%U', W='%W' %% bla",&date);
	printf("%s\n",str);

	return 0;
}

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
