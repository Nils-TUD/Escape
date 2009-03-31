/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/date.h>
#include <esc/fileio.h>

#define MAX_DATE_LEN 100

int main(int argc,char **argv) {
	u32 ts;
	sDate date;
	char *fmt = "%c";
	char str[MAX_DATE_LEN];

	/* use format from argument? */
	if(argc == 2)
		fmt = argv[1];

	getDate(&date);
	dateToString(str,MAX_DATE_LEN,fmt,&date);
	printf("%s\n",str);

	return 0;
}
