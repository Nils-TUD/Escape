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

#include <common.h>

class RTC {
	RTC() = delete;

	enum {
		PORT_IDX		= 0x70,
		PORT_DATA		= 0x71
	};

	/* all in BCD */
	enum {
		REG_SEC			= 0x0,	/* 00-59 */
		REG_MIN			= 0x2,	/* 00-59 */
		REG_HOUR		= 0x4,	/* 00-23 */
		REG_WEEKDAY		= 0x6,	/* 01-07; Sunday=1 */
		REG_MONTHDAY	= 0x7,	/* 01-31 */
		REG_MONTH		= 0x8,	/* 01-12 */
		REG_YEAR		= 0x9,	/* 00-99 */
		REG_STATUSA		= 0xA,
		REG_STATUSB		= 0xB,
	};

public:
	static void init();
	static time_t getTime();

private:
	static uint decodeBCD(uint8_t val);
	static uint8_t read(uint8_t reg);
};
