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
#include <esc/proc.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <esc/sync.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static int refreshThread(void *arg);

static tUserSem usem;
static sMsg msg;
static sRTCInfo info;
static time_t timestamp;

int main(void) {
	msgid_t mid;
	int id;

	if(usemcrt(&usem,1) < 0)
		error("Unable to create lock");
	if(startthread(refreshThread,NULL) < 0)
		error("Unable to start RTC-thread");

	id = createdev("/dev/rtc",0440,DEV_TYPE_BLOCK,DEV_READ | DEV_CLOSE);
	if(id < 0)
		error("Unable to register device 'rtc'");

	/* wait for commands */
	while(1) {
		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_READ: {
					uint offset = msg.args.arg1;
					uint count = msg.args.arg2;
					msg.args.arg1 = count;
					msg.args.arg2 = READABLE_DONT_SET;
					if(offset + count <= offset || offset + count > sizeof(sRTCInfo))
						msg.args.arg1 = 0;
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1) {
						/* ensure that the refresh-thread doesn't access the date in the
						 * meanwhile */
						usemdown(&usem);
						send(fd,MSG_DEV_READ_RESP,(uchar*)&info + offset,msg.args.arg1);
						usemup(&usem);
					}
				}
				break;

				case MSG_DEV_CLOSE:
					close(fd);
					break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}

	/* clean up */
	close(id);
	return EXIT_SUCCESS;
}

static int refreshThread(A_UNUSED void *arg) {
	struct tm *gmt;
	while(1) {
		/* ensure that the driver-loop doesn't access the date in the meanwhile */
		usemdown(&usem);
		timestamp++;
		gmt = gmtime(&timestamp);
		memcpy(&info.time,gmt,sizeof(struct tm));
		usemup(&usem);
		sleep(1000);
	}
	return 0;
}
