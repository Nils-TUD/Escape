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
#include <esc/proc.h>
#include <esc/service.h>
#include <messages.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/ports.h>
#include <esc/date.h>
#include <stdlib.h>

#define IOPORT_CMOS_INDEX	0x70
#define IOPORT_CMOS_DATA	0x71

/* all in BCD */
#define CMOS_REG_SEC		0x0		/* 00-59 */
#define CMOS_REG_MIN		0x2		/* 00-59 */
#define CMOS_REG_HOUR		0x4		/* 00-23 */
#define CMOS_REG_WEEKDAY	0x6		/* 01-07; Sunday=1 */
#define CMOS_REG_MONTHDAY	0x7		/* 01-31 */
#define CMOS_REG_MONTH		0x8		/* 01-12 */
#define CMOS_REG_YEAR		0x9		/* 00-99 */

static void cmos_refresh(void);
static u32 cmos_decodeBCD(u8 val);
static u8 cmos_read(u8 reg);

static tFD dateFD;

int main(void) {
	/* request io-ports */
	if(requestIOPorts(IOPORT_CMOS_INDEX,2) < 0) {
		printe("Unable to request io-ports %d .. %d",IOPORT_CMOS_INDEX,IOPORT_CMOS_INDEX + 1);
		return EXIT_FAILURE;
	}

	/* create a date-node */
	if(createNode("system:/bin/date") < 0) {
		printe("Unable to create node system:/bin/date");
		return EXIT_FAILURE;
	}

	dateFD = open("system:/bin/date",IO_READ | IO_WRITE);
	if(dateFD < 0) {
		printe("Unable to open system:/bin/date");
		return EXIT_FAILURE;
	}

	/* refresh once per second */
	while(1) {
		cmos_refresh();
		sleep(1000);
	}

	/* clean up */
	close(dateFD);
	releaseIOPorts(IOPORT_CMOS_INDEX,2);
	return EXIT_SUCCESS;
}

static void cmos_refresh(void) {
	sDate date;
	date.monthDay = cmos_decodeBCD(cmos_read(CMOS_REG_MONTHDAY));
	date.month = cmos_decodeBCD(cmos_read(CMOS_REG_MONTH));
	date.year = cmos_decodeBCD(cmos_read(CMOS_REG_YEAR)) + 2000;
	date.hour = cmos_decodeBCD(cmos_read(CMOS_REG_HOUR));
	date.min = cmos_decodeBCD(cmos_read(CMOS_REG_MIN));
	date.sec = cmos_decodeBCD(cmos_read(CMOS_REG_SEC));
	date.weekDay = cmos_decodeBCD(cmos_read(CMOS_REG_WEEKDAY)) + 1;

	seek(dateFD,0);
	write(dateFD,&date,sizeof(sDate));
}

static u32 cmos_decodeBCD(u8 val) {
	return (val >> 4) * 10 + (val & 0xF);
}

static u8 cmos_read(u8 reg) {
	outByte(IOPORT_CMOS_INDEX,reg);
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	return inByte(IOPORT_CMOS_DATA);
}
