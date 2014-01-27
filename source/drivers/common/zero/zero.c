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
#include <esc/messages.h>
#include <esc/driver.h>
#include <esc/mem.h>
#include <esc/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#define BUF_SIZE		(16 * 1024)
#define MAX_CLIENT_FD	256

static sMsg msg;
static char zeros[BUF_SIZE];
static char *shbufs[MAX_CLIENT_FD];

int main(void) {
	int id;
	msgid_t mid;

	id = createdev("/dev/zero",0444,DEV_TYPE_CHAR,DEV_SHFILE | DEV_READ | DEV_CLOSE);
	if(id < 0)
		error("Unable to register device 'zero'");

	/* 0's are always available ;) */
	if(fcntl(id,F_SETDATA,true) < 0)
		error("fcntl");

	/* wait for commands */
	while(1) {
		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
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
					size_t size = msg.args.arg1;
					char *path = msg.str.s1;
					assert(shbufs[fd] == NULL);
					shbufs[fd] = joinbuf(path,size,0);
					msg.args.arg1 = shbufs[fd] != NULL ? 0 : -errno;
					send(fd,MSG_DEV_SHFILE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_READ: {
					void *data = NULL;
					/* offset is ignored here */
					size_t count = msg.args.arg2;
					ssize_t shmemoff = msg.args.arg3;
					msg.args.arg2 = READABLE_DONT_SET;
					if(shmemoff != -1) {
						assert(shbufs[fd] != NULL);
						memset(shbufs[fd] + shmemoff,0,count);
					}
					else {
						data = count <= BUF_SIZE ? zeros : calloc(count,1);
						if(!data) {
							printe("Unable to alloc mem");
							count = 0;
						}
					}
					msg.args.arg1 = count;
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(shmemoff == -1 && count) {
						send(fd,MSG_DEV_READ_RESP,data,count);
						if(count > BUF_SIZE)
							free(data);
					}
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
	close(id);
	return EXIT_SUCCESS;
}
