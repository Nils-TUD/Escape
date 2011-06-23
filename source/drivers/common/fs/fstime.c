/**
 * $Id$
 */

#include <esc/common.h>
#include <stdio.h>
#include <time.h>
/* TODO better solution! */
#include "../../../lib/c/time/timeintern.h"

static tFD timeFd = -1;

tTime timestamp(void) {
	struct tm time;
	/* open CMOS and read date */
	int err;
	if(timeFd < 0) {
		/* not already open, so do it */
		timeFd = open(TIME_DRIVER,IO_READ);
		if(timeFd < 0)
			return 0;
	}
	else {
		/* seek back to beginning */
		if(seek(timeFd,0,SEEK_SET) < 0)
			return 0;
	}
	if(RETRY(read(timeFd,&time,sizeof(struct tm))) < 0)
		return 0;
	return mktime(&time);
}
