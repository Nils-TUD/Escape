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
#include <esc/arch/i586/ports.h>
#include <esc/proc.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <stdio.h>
#include <errors.h>
#include <stdlib.h>
#include <time.h>

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

static int refreshThread(void *arg);
static void cmos_refresh(void);
static uint cmos_decodeBCD(uint8_t val);
static uint8_t cmos_read(uint8_t reg);

static tULock dlock;
static sMsg msg;
static struct tm date;

int main(void) {
	msgid_t mid;
	int id;

	/* request io-ports */
	if(requestIOPorts(IOPORT_CMOS_INDEX,2) < 0)
		error("Unable to request io-ports %d .. %d",IOPORT_CMOS_INDEX,IOPORT_CMOS_INDEX + 1);

	if(startThread(refreshThread,NULL) < 0)
		error("Unable to start CMOS-thread");

	id = regDriver("cmos",DRV_READ);
	if(id < 0)
		error("Unable to register driver 'cmos'");

	/* there is always data available */
	if(fcntl(id,F_SETDATA,true) < 0)
		error("fcntl");

	/* wait for commands */
	while(1) {
		int fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[CMOS] Unable to get work");
		else {
			switch(mid) {
				case MSG_DRV_READ: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					msg.args.arg1 = count;
					msg.args.arg2 = true;
					if(offset + count <= offset || offset + count > sizeof(struct tm))
						msg.args.arg1 = 0;
					send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1) {
						/* ensure that the refresh-thread doesn't access the date in the
						 * meanwhile */
						locku(&dlock);
						send(fd,MSG_DRV_READ_RESP,(uchar*)&date + offset,count);
						unlocku(&dlock);
					}
				}
				break;

				default:
					msg.args.arg1 = ERR_UNSUPPORTED_OP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	/* clean up */
	close(id);
	releaseIOPorts(IOPORT_CMOS_INDEX,2);
	return EXIT_SUCCESS;
}

static int refreshThread(void *arg) {
	UNUSED(arg);
	while(1) {
		/* ensure that the driver-loop doesn't access the date in the meanwhile */
		locku(&dlock);
		cmos_refresh();
		unlocku(&dlock);
		sleep(1000);
	}
	return 0;
}

static void cmos_refresh(void) {
	date.tm_mday = cmos_decodeBCD(cmos_read(CMOS_REG_MONTHDAY)) - 1;
	date.tm_mon = cmos_decodeBCD(cmos_read(CMOS_REG_MONTH)) - 1;
	date.tm_year = cmos_decodeBCD(cmos_read(CMOS_REG_YEAR));
	if(date.tm_year < 70)
		date.tm_year += 100;
	date.tm_hour = cmos_decodeBCD(cmos_read(CMOS_REG_HOUR));
	date.tm_min = cmos_decodeBCD(cmos_read(CMOS_REG_MIN));
	date.tm_sec = cmos_decodeBCD(cmos_read(CMOS_REG_SEC));
	date.tm_wday = cmos_decodeBCD(cmos_read(CMOS_REG_WEEKDAY)) - 1;
}

static uint cmos_decodeBCD(uint8_t val) {
	return (val >> 4) * 10 + (val & 0xF);
}

static uint8_t cmos_read(uint8_t reg) {
	outByte(IOPORT_CMOS_INDEX,reg);
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	return inByte(IOPORT_CMOS_DATA);
}
