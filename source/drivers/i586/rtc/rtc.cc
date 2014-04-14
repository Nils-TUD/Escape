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

#include <esc/common.h>
#include <esc/arch/i586/ports.h>
#include <esc/proc.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <esc/time.h>
#include <ipc/proto/file.h>
#include <ipc/proto/rtc.h>
#include <ipc/device.h>
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

static void rtc_readInfo(ipc::RTC::Info *info);
static uint rtc_decodeBCD(uint8_t val);
static uint8_t rtc_read(uint8_t reg);

class RTCDevice : public ipc::Device {
public:
	explicit RTCDevice(const char *path,mode_t mode)
		: ipc::Device(path,mode,DEV_TYPE_BLOCK,DEV_READ) {
		set(MSG_FILE_READ,std::make_memfun(this,&RTCDevice::read));
	}

	void read(ipc::IPCStream &is) {
		ipc::FileRead::Request r;
		is >> r;

		ssize_t res = r.count;
		if(r.offset + r.count <= r.offset)
			res = -EINVAL;
		else if(r.offset > sizeof(ipc::RTC::Info))
			res = 0;
		else if(r.offset + r.count > sizeof(ipc::RTC::Info))
			res = sizeof(ipc::RTC::Info) - r.offset;

		is << ipc::FileRead::Response(res) << ipc::Reply();
		if(res > 0) {
			/* we assume that the system booted at X secs + 0 us, because we don't know
			 * the microseconds at boot-time. */
			ipc::RTC::Info info;
			rtc_readInfo(&info);
			info.microsecs = (uint)(tsctotime(rdtsc())) % 1000000;
			is << ipc::ReplyData((uchar*)&info + r.offset,res);
		}
	}
};

int main(void) {
	/* request io-ports */
	if(reqports(IOPORT_CMOS_INDEX,2) < 0)
		error("Unable to request io-ports %d .. %d",IOPORT_CMOS_INDEX,IOPORT_CMOS_INDEX + 1);

	RTCDevice dev("/dev/rtc",0444);
	dev.loop();

	/* clean up */
	relports(IOPORT_CMOS_INDEX,2);
	return EXIT_SUCCESS;
}

static void rtc_readInfo(ipc::RTC::Info *info) {
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
