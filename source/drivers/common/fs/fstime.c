/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/driver.h>
#include <stdio.h>
#include <time.h>
#include "fstime.h"

static int timeFd = -1;

time_t timestamp(void) {
	struct tm t;
	/* open CMOS and read date */
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
	if(RETRY(read(timeFd,&t,sizeof(struct tm))) < 0)
		return 0;
	return mktime(&t);
}
