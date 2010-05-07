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
#include <time.h>
#include "timeintern.h"

void datetotm(struct tm *timeptr,const sDate *date) {
	timeptr->tm_hour = date->hour;
	timeptr->tm_min = date->min;
	timeptr->tm_sec = date->sec;
	timeptr->tm_mon = date->month - 1;
	timeptr->tm_mday = date->monthDay;
	timeptr->tm_wday = date->weekDay - 1;
	timeptr->tm_yday = date->yearDay;
	timeptr->tm_year = date->year - 1900;
	timeptr->tm_isdst = date->isDst;
}
