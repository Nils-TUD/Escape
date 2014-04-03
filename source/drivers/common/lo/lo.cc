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
#include <esc/driver/pci.h>
#include <esc/messages.h>
#include <esc/driver.h>
#include <esc/mem.h>
#include <esc/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <list>

#define MAX_CLIENT_FD		8

struct Packet {
	void *data;
	size_t size;
};

static char *shbufs[MAX_CLIENT_FD];
static std::list<Packet> packets;

int main(void) {
	/* reg device */
	int sid = createdev("/dev/lo",0770,DEV_TYPE_CHAR,
		DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_WRITE | DEV_CLOSE);
	if(sid < 0)
		error("Unable to register device 'mouse'");

	while(1) {
		sMsg msg;
		msgid_t mid;
		int fd = getwork(sid,&mid,&msg,sizeof(msg),0);
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
					shbufs[fd] = (char*)joinbuf(path,size,0);
					msg.args.arg1 = shbufs[fd] != NULL ? 0 : -errno;
					send(fd,MSG_DEV_SHFILE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_READ: {
					void *data = NULL;
					/* offset is ignored here */
					size_t count = msg.args.arg2;
					ssize_t shmemoff = msg.args.arg3;
					if(shmemoff != -1)
						data = shbufs[fd] + shmemoff;
					else {
						data = malloc(count);
						if(!data) {
							printe("Unable to alloc mem");
							count = 0;
						}
					}

					assert(packets.size() > 0);

					Packet pkt = packets.front();
					msg.args.arg1 = count >= pkt.size ? pkt.size : -ENOMEM;
					if(shmemoff != -1 && msg.args.arg1)
						memcpy(data,pkt.data,pkt.size);

					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(shmemoff == -1 && msg.args.arg1) {
						send(fd,MSG_DEV_READ_RESP,pkt.data,msg.args.arg1);
						free(data);
					}

					free(pkt.data);
					packets.pop_back();
				}
				break;

				case MSG_DEV_WRITE: {
					/* offset is ignored here */
					size_t count = msg.args.arg2;
					ssize_t shmemoff = msg.args.arg3;

					Packet pkt;
					pkt.data = malloc(count);
					pkt.size = count;

					if(pkt.data) {
						if(shmemoff == -1) {
							if(receive(fd,NULL,pkt.data,count) != (ssize_t)count)
								printe("Unable to receive data");
						}
						else
							memcpy(pkt.data,shbufs[fd] + shmemoff,count);
						packets.push_back(pkt);
						fcntl(sid,F_WAKE_READER,0);
					}

					msg.args.arg1 = pkt.data ? count : -ENOMEM;
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
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

				case MSG_NIC_GETMAC: {
					msg.args.arg1 = 0;
					msg.args.arg2 = 0;
					msg.args.arg3 = 0;
					msg.args.arg4 = 0;
					msg.args.arg5 = 0;
					msg.args.arg6 = 0;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}
	close(sid);
	return EXIT_SUCCESS;
}
