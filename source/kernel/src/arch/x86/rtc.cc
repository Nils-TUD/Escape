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

#include <common.h>
#include <arch/x86/rtc.h>
#include <arch/x86/ports.h>
#include <time.h>

time_t RTC::getTime() {
	struct tm time;
	time.tm_mday = decodeBCD(read(REG_MONTHDAY)) - 1;
	time.tm_mon = decodeBCD(read(REG_MONTH)) - 1;
	time.tm_year = decodeBCD(read(REG_YEAR));
	if(time.tm_year < 70)
		time.tm_year += 100;
	time.tm_hour = decodeBCD(read(REG_HOUR));
	time.tm_min = decodeBCD(read(REG_MIN));
	time.tm_sec = decodeBCD(read(REG_SEC));
	time.tm_wday = decodeBCD(read(REG_WEEKDAY)) - 1;
	return mktime(&time);
}

uint RTC::decodeBCD(uint8_t val) {
	return (val >> 4) * 10 + (val & 0xF);
}

uint8_t RTC::read(uint8_t reg) {
	Ports::out<uint8_t>(PORT_IDX,reg);
	asm volatile ("nop");
	asm volatile ("nop");
	asm volatile ("nop");
	return Ports::in<uint8_t>(PORT_DATA);
}
