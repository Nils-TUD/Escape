/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#include <esc/common.h>

/* 32-bit integer that is stored in little- and bigendian */
struct ISOInt32 {
	uint32_t littleEndian;
	uint32_t bigEndian;
};

/* 16-bit integer that is stored in little- and bigendian */
struct ISOInt16 {
	uint16_t littleEndian;
	uint16_t bigEndian;
};

/* dates how they are stored in directory-entires */
struct ISODirDate {
	uint8_t year;				/* Number of years since 1900. */
	uint8_t month;				/* Month of the year from 1 to 12. */
	uint8_t day;					/* Day of the month from 1 to 31. */
	uint8_t hour;				/* Hour of the day from 0 to 23. */
	uint8_t minute;				/* Minute of the hour from 0 to 59. */
	uint8_t second;				/* Second of the minute from 0 to 59. */
	uint8_t offset;				/* Offset from GMT in 15 minute intervals from -48 (West) to +52 (East). */
} A_PACKED;

/* dates how they are stored in the volume-descriptor */
struct ISOVolDate {
	char year[4];			/* Year from 1 to 9999. */
	char month[2];			/* Month from 1 to 12. */
	char day[2];			/* Day from 1 to 31. */
	char hour[2];			/* Hour from 0 to 23. */
	char minute[2];			/* Minute from 0 to 59. */
	char second[2];			/* Second from 0 to 59. */
	char second100ths[2];	/* Hundredths of a second from 0 to 99. */
	uint8_t offset;				/* Offset from GMT in 15 minute intervals from -48 (West) to +52 (East) */
} A_PACKED;
