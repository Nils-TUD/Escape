/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
		timeFd = open(TIME_DEVICE,IO_READ);
		if(timeFd < 0)
			return 0;
	}
	else {
		/* seek back to beginning */
		if(seek(timeFd,0,SEEK_SET) < 0)
			return 0;
	}
	if(IGNSIGS(read(timeFd,&t,sizeof(struct tm))) < 0)
		return 0;
	return mktime(&t);
}
