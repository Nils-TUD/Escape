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
#include <esc/io.h>
#include <esc/proc.h>
#include <esc/driver.h>
#include <stdlib.h>
#include <stdio.h>
#include <esc/messages.h>
#include "driver.h"

static sMsg msg;

int mod_driver(int argc,char *argv[]) {
	bool quit = false;
	tMsgId mid;
	tDrvId id;
	UNUSED(argc);
	UNUSED(argv);

	id = regDriver("bla",DRV_OPEN | DRV_READ | DRV_WRITE | DRV_CLOSE);
	if(id < 0)
		printe("regDriver");

	if(fork() == 0) {
		char buf[10] = {0};
		tFD fd = open("/dev/bla",IO_READ | IO_WRITE);
		if(fd < 0)
			printe("open");
		printf("Reading...\n");
		if(read(fd,buf,10) < 0)
			printe("read");
		printf("Res: %s\n",buf);
		printf("Writing %s...\n",buf);
		if(write(fd,buf,10) < 0)
			printe("write");
		printf("Closing...\n");
		close(fd);
		return EXIT_SUCCESS;
	}

	setDataReadable(id,true);
	while(!quit) {
		tFD cfd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(cfd < 0)
			printe("[TEST] Unable to get work");
		else {
			switch(mid) {
				case MSG_DRV_OPEN:
					printf("Open: flags=%d\n",msg.args.arg1);
					msg.args.arg1 = 0;
					send(cfd,MSG_DRV_OPEN_RESP,&msg,sizeof(msg.args));
					break;
				case MSG_DRV_READ:
					printf("Read: offset=%d, count=%d\n",msg.args.arg1,msg.args.arg2);
					msg.args.arg1 = msg.args.arg2;
					msg.args.arg2 = true;
					send(cfd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
					send(cfd,MSG_DRV_READ_RESP,"test!!",SSTRLEN("test!!"));
					break;
				case MSG_DRV_WRITE: {
					char *buf = (char*)malloc(msg.args.arg2);
					printf("Write: offset=%d, count=%d\n",msg.args.arg1,msg.args.arg2);
					msg.args.arg1 = 0;
					if(receive(cfd,&mid,buf,msg.args.arg2) >= 0) {
						printf("Got %s\n",buf);
						msg.args.arg1 = msg.args.arg2;
					}
					send(cfd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
					free(buf);
				}
				break;
				case MSG_DRV_CLOSE:
					printf("Close\n");
					quit = true;
					break;
				default:
					printf("Unknown command");
					break;
			}
			close(cfd);
		}
	}

	unregDriver(id);
	return EXIT_SUCCESS;
}
