/**
 * $Id: randmain.c 1091 2011-11-12 22:08:05Z nasmussen $
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
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <esc/messages.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define MAX_CLIENT_FD	256

static sMsg msg;
static char *shbufs[MAX_CLIENT_FD];

int main(int argc,char **argv) {
	int id,fd;
	char *addr;
	size_t size;
	sFileInfo info;

	if(argc != 3)
		error("Usage: %s <wait> <module>",argv[0]);

	/* mmap the module */
	if(stat(argv[2],&info) < 0)
		error("Unable to stat '%s'",argv[2]);
	fd = open(argv[2],IO_READ);
	if(fd < 0)
		error("Unable to open module '%s'",argv[2]);
	size = info.size;
	addr = mmap(NULL,size,size,PROT_READ,MAP_PRIVATE,fd,0);
	if(!addr)
		error("Unable to map file '%s'",argv[2]);

	/* create device */
	id = createdev("/dev/romdisk",0400,DEV_TYPE_BLOCK,DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_CLOSE);
	if(id < 0)
		error("Unable to register device 'romdisk'");

	/* data is always available */
	if(fcntl(id,F_SETDATA,true) < 0)
		error("fcntl");

	/* wait for commands */
	while(1) {
		msgid_t mid;
		fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_OPEN: {
					msg.args.arg1 = fd < MAX_CLIENT_FD ? 0 : -ENOMEM;
					send(fd,MSG_DEV_OPEN_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_SHFILE: {
					size_t shmsize = msg.args.arg1;
					char *path = msg.str.s1;
					assert(shbufs[fd] == NULL);
					shbufs[fd] = joinbuf(path,shmsize);
					msg.args.arg1 = shbufs[fd] != NULL;
					send(fd,MSG_DEV_SHFILE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_READ: {
					ulong offset = msg.args.arg1;
					ulong count = msg.args.arg2;
					ssize_t shmemoff = msg.args.arg3;
					msg.args.arg1 = 0;
					if(offset + count <= size && offset + count > offset)
						msg.data.arg1 = count;
					msg.args.arg2 = READABLE_DONT_SET;
					if(shmemoff != -1)
						memcpy(shbufs[fd] + shmemoff,addr + offset,msg.data.arg1);
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(shmemoff == -1 && msg.args.arg1 > 0)
						send(fd,MSG_DEV_READ_RESP,addr + offset,msg.data.arg1);
				}
				break;

				case MSG_DISK_GETSIZE: {
					msg.args.arg1 = size;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_CLOSE: {
					if(shbufs[fd]) {
						munmap(shbufs[fd]);
						shbufs[fd] = NULL;
					}
					close(fd);
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}

	/* clean up */
	munmap(addr);
	close(id);
	return EXIT_SUCCESS;
}
