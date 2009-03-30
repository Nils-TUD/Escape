/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <time.h>
#include <bufio.h>

static const char *weekDays[] = {
	"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

int main(void) {
	u32 ts;
	sDate date;

	ts = getTime();
	getDate(&date);

	printf("UNIX-Timestamp: %d\n",ts);
	printf("%s, %02d/%02d/%04d, %02d:%02d:%02d\n",weekDays[date.weekDay],
			date.month,date.monthDay,date.year,date.hour,date.min,date.sec);
	return 0;
}
