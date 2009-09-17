/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/date.h>
#include <esc/io.h>
#include "dateintern.h"

s32 getDate(sDate *date) {
	s32 err;
	u8 yearType;
	s32 m;

	/* read date provided by CMOS */
	tFD fd = open("/system/date",IO_READ);
	if(fd < 0)
		return fd;
	if((err = read(fd,date,sizeof(sDate))) < 0)
		return err;
	close(fd);

	/* set year-day */
	date->yearDay = date->monthDay - 1;
	yearType = IS_LEAP_YEAR(date->year) ? LEAP_YEAR : DEF_YEAR;
	for(m = (s32)date->month - 2; m >= 0; m--)
		date->yearDay += daysPerMonth[yearType][m];

	return 0;
}
