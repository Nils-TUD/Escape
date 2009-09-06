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
#include <esc/fileio.h>
#include <esc/proc.h>
#include <esc/service.h>
#include <messages.h>
#include <stdlib.h>
#include "driver.h"

int mod_driver(int argc,char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	tServ client;
	tServ id = regService("bla",SERV_DRIVER);
	if(id < 0)
		printe("regService");

	if(fork() == 0) {
		char buf[10] = {0};
		tFD fd = open("/drivers/bla",IO_READ | IO_WRITE);
		if(fd < 0)
			printe("open");
		printf("Reading...\n");
		read(fd,buf,10);
		printf("Res: %s\n",buf);
		printf("Writing %s...\n",buf);
		write(fd,buf,10);
		printf("IOCtl read\n");
		ioctl(fd,1,(u8*)buf,10);
		printf("Got '%s'\n",buf);
		printf("IOCtl write\n");
		ioctl(fd,0,(u8*)buf,10);
		printf("Closing...\n");
		close(fd);
		return EXIT_SUCCESS;
	}

	setDataReadable(id,true);
	bool quit = false;
	tMsgId mid;
	static sMsg msg;
	while(!quit) {
		tFD cfd = getClient(&id,1,&client);
		if(cfd < 0)
			wait(EV_CLIENT | EV_RECEIVED_MSG);
		else {
			while(receive(cfd,&mid,&msg,sizeof(msg)) > 0) {
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
						send(cfd,MSG_DRV_READ_RESP,"test!!",strlen("test!!"));
						break;
					case MSG_DRV_WRITE: {
						char *buf = (char*)malloc(msg.args.arg2);
						printf("Write: offset=%d, count=%d\n",msg.args.arg1,msg.args.arg2);
						receive(cfd,&mid,buf,msg.args.arg2);
						printf("Got %s\n",buf);
						msg.args.arg1 = msg.args.arg2;
						send(cfd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
						free(buf);
					}
					break;
					case MSG_DRV_IOCTL: {
						u32 cmd = msg.data.arg2;
						printf("Ioctl: cmd=%d, dsize=%d\n",msg.data.arg1,msg.data.arg2);
						msg.data.arg1 = 0;
						if(cmd == 0) {
							msg.data.arg2 = 0;
							printf("Got '%s'\n",msg.data.d);
						}
						else {
							msg.data.arg2 = 9;
							strcpy(msg.data.d,"test1234");
						}
						send(cfd,MSG_DRV_IOCTL_RESP,&msg,sizeof(msg.data));
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
			}
			close(cfd);
		}
	}

	unregService(id);
	return EXIT_SUCCESS;
}
