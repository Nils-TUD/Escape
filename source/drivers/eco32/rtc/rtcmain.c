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
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <stdio.h>
#include <errors.h>
#include <error.h>
#include <string.h>
#include <time.h>

static int refreshThread(void *arg);

static tULock dlock;
static sMsg msg;
static struct tm date;
static time_t timestamp;

int main(void) {
	msgid_t mid;
	int id;

	if(startThread(refreshThread,NULL) < 0)
		error("Unable to start RTC-thread");

	id = regDriver("rtc",DRV_READ);
	if(id < 0)
		error("Unable to register driver 'rtc'");

	/* there is always data available */
	if(fcntl(id,F_SETDATA,true) < 0)
		error("fcntl");

	/* wait for commands */
	while(1) {
		int fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[RTC] Unable to get work");
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
	return EXIT_SUCCESS;
}

static int refreshThread(void *arg) {
	UNUSED(arg);
	struct tm *gmt;
	while(1) {
		/* ensure that the driver-loop doesn't access the date in the meanwhile */
		locku(&dlock);
		timestamp++;
		gmt = gmtime(&timestamp);
		memcpy(&date,gmt,sizeof(struct tm));
		unlocku(&dlock);
		sleep(1000);
	}
	return 0;
}

