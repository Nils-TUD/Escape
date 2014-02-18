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
#include <esc/io.h>
#include <esc/proc.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <esc/sync.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include "../modules.h"

static int clientThread(A_UNUSED void *arg) {
	int buf;
	int fd = open("/dev/bla",IO_READ | IO_WRITE);
	if(fd < 0)
		error("Unable to open /dev/bla");
	ssize_t res;
	while((res = read(fd,&buf,sizeof(buf))) > 0)
		;
	printe("Got res=%zd",res);
	close(fd);
	return EXIT_SUCCESS;
}

int mod_driverread(A_UNUSED int argc,A_UNUSED char *argv[]) {
	int id = createdev("/dev/bla",0666,DEV_TYPE_CHAR,DEV_READ | DEV_CLOSE);
	if(id < 0)
		error("createdev");
	if(startthread(clientThread,NULL) < 0)
		error("startthread");

	if(fcntl(id,F_WAKE_READER,0) < 0)
		error("fcntl");
	if(fcntl(id,F_WAKE_READER,0) < 0)
		error("fcntl");

	sMsg msg;
	msgid_t mid;
	int resp = 4;
	for(int i = 0; i < 2; ++i) {
		int cfd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(cfd < 0)
			printe("getwork failed");
		else {
			switch(mid) {
				case MSG_DEV_READ:
					msg.args.arg1 = msg.args.arg2;
					send(cfd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					send(cfd,MSG_DEV_READ_RESP,&resp,sizeof(resp));
					break;
				case MSG_DEV_CLOSE:
					close(cfd);
					break;
			}
		}
	}

	fprintf(stderr,"Waiting a second...\n");
	sleep(1000);
	fprintf(stderr,"Closing device\n");
	close(id);
	fprintf(stderr,"Waiting for client to notice it...\n");
	join(0);
	fprintf(stderr,"Shutting down.\n");
	return EXIT_SUCCESS;
}
