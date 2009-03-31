/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/date.h>
#include <esc/fileio.h>
#include <stdlib.h>

#define MAX_DATE_LEN 100

int main(int argc,char **argv) {
	u32 ts;
	sDate date;
	char *fmt = "%c";
	char str[MAX_DATE_LEN];

	/* use format from argument? */
	if(argc == 2)
		fmt = argv[1];

	if(getDate(&date) < 0) {
		printe("Unable to get date");
		return EXIT_FAILURE;
	}

	if(dateToString(str,MAX_DATE_LEN,fmt,&date) == 0) {
		fprintf(stderr,"Unable to format date\n");
		return EXIT_FAILURE;
	}

	printf("%s\n",str);
	return EXIT_SUCCESS;
}
