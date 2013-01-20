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
#include <esc/arch/i586/ports.h>
#include <esc/proc.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <esc/time.h>
#include <stdio.h>
#include <errno.h>
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

static void rtc_readInfo(sRTCInfo *info);
static uint rtc_decodeBCD(uint8_t val);
static uint8_t rtc_read(uint8_t reg);

static sMsg msg;

int main(void) {
	msgid_t mid;
	int id;

	/* request io-ports */
	if(reqports(IOPORT_CMOS_INDEX,2) < 0)
		error("Unable to request io-ports %d .. %d",IOPORT_CMOS_INDEX,IOPORT_CMOS_INDEX + 1);

	id = createdev("/dev/rtc",DEV_TYPE_BLOCK,DEV_READ);
	if(id < 0)
		error("Unable to register device 'rtc'");

	/* give all read- and write-permission */
	if(chmod("/dev/rtc",S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0)
		error("Unable to set permissions for /dev/rtc");

	/* wait for commands */
	while(1) {
		int fd = getwork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[RTC] Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_READ: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					msg.args.arg1 = count;
					msg.args.arg2 = true;
					if(offset + count <= offset || offset + count > sizeof(sRTCInfo))
						msg.args.arg1 = 0;
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1) {
						/* we assume that the system booted at X secs + 0 us, because we don't know
						 * the microseconds at boot-time. */
						sRTCInfo info;
						rtc_readInfo(&info);
						info.microsecs = (uint)(tsctotime(rdtsc())) % 1000000;
						send(fd,MSG_DEV_READ_RESP,(uchar*)&info + offset,msg.args.arg1);
					}
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	/* clean up */
	close(id);
	relports(IOPORT_CMOS_INDEX,2);
	return EXIT_SUCCESS;
}

static void rtc_readInfo(sRTCInfo *info) {
	info->time.tm_mday = rtc_decodeBCD(rtc_read(CMOS_REG_MONTHDAY)) - 1;
	info->time.tm_mon = rtc_decodeBCD(rtc_read(CMOS_REG_MONTH)) - 1;
	info->time.tm_year = rtc_decodeBCD(rtc_read(CMOS_REG_YEAR));
	if(info->time.tm_year < 70)
		info->time.tm_year += 100;
	info->time.tm_hour = rtc_decodeBCD(rtc_read(CMOS_REG_HOUR));
	info->time.tm_min = rtc_decodeBCD(rtc_read(CMOS_REG_MIN));
	info->time.tm_sec = rtc_decodeBCD(rtc_read(CMOS_REG_SEC));
	info->time.tm_wday = rtc_decodeBCD(rtc_read(CMOS_REG_WEEKDAY)) - 1;
}

static uint rtc_decodeBCD(uint8_t val) {
	return (val >> 4) * 10 + (val & 0xF);
}

static uint8_t rtc_read(uint8_t reg) {
	outbyte(IOPORT_CMOS_INDEX,reg);
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	__asm__ volatile ("nop");
	return inbyte(IOPORT_CMOS_DATA);
}
