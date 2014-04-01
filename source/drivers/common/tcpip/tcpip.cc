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
#include <esc/thread.h>
#include <esc/driver.h>
#include <esc/sync.h>
#include <stdio.h>
#include <iostream>
#include <sstream>

#include "ethernet.h"
#include "nic.h"
#include "arp.h"
#include "icmp.h"
#include "route.h"
#include "udp.h"

#define MAX_CLIENT_FD	128

static tUserSem sem;
static Socket *sockets[MAX_CLIENT_FD];

static void printPacket(uint8_t *bytes,size_t count) {
	for(size_t i = 0; i < count; ++i) {
		printf("%02x ",bytes[i]);
		if(i % 8 == 7)
			printf("\n");
	}
	printf("\n");
	fflush(stdout);
}

static int receiveThread(void *arg) {
	static uint8_t buffer[4096];
	NIC *nic = reinterpret_cast<NIC*>(arg);
	while(1) {
		ssize_t res = nic->read(buffer,sizeof(buffer));
		if(res < 0) {
			printe("Reading packet failed");
			continue;
		}

		std::cout << "[" << nic->mac() << "] Received packet with " << res << " bytes" << std::endl;
		if((size_t)res >= sizeof(Ethernet<>)) {
			usemdown(&sem);
			Ethernet<> *packet = reinterpret_cast<Ethernet<>*>(buffer);
			std::cout << *packet << std::endl;
			ssize_t err = Ethernet<>::receive(*nic,packet,res);
			if(err < 0)
				printe("Invalid packet: %s",strerror(-err));
			usemup(&sem);
		}
		else
			printe("Ignoring packet of size %zd",res);
	}
	return 0;
}

static int udpThread(void*) {
	/* reg device */
	int sid = createdev("/dev/udp",0777,DEV_TYPE_BLOCK,
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
					IPv4Addr ip;
					port_t port = 0;
					std::istringstream sip(msg.str.s1);
					sip >> ip >> port;

					msg.args.arg1 = -ENOMEM;
					if(fd < MAX_CLIENT_FD) {
						msg.args.arg1 = 0;
						sockets[fd] = new UDPSocket(fd,ip,port);
					}
					send(fd,MSG_DEV_OPEN_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_SHFILE: {
					size_t size = msg.args.arg1;
					char *path = msg.str.s1;
					sockets[fd]->shm((char*)joinbuf(path,size,0));
					msg.args.arg1 = sockets[fd]->shm() != NULL ? 0 : -errno;
					send(fd,MSG_DEV_SHFILE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_WRITE: {
					void *data = NULL;
					/* offset is ignored here */
					size_t count = msg.args.arg2;
					ssize_t shmemoff = msg.args.arg3;

					if(shmemoff == -1) {
						data = malloc(count);
						if(data) {
							if(receive(fd,NULL,data,count) != (ssize_t)count)
								printe("Unable to receive data");
						}
					}
					else
						data = sockets[fd]->shm() + shmemoff;

					usemdown(&sem);
					msg.args.arg1 = data ? sockets[fd]->sendto(NULL,data,count) : -ENOMEM;
					usemup(&sem);
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
					if(shmemoff == -1)
						free(data);
				}
				break;

				case MSG_DEV_READ: {
					void *data = NULL;
					/* offset is ignored here */
					size_t count = msg.args.arg2;
					ssize_t shmemoff = msg.args.arg3;
					if(shmemoff != -1)
						data = sockets[fd]->shm() + shmemoff;

					usemdown(&sem);
					ssize_t res = sockets[fd]->receive(data,count);
					usemup(&sem);
					if(res < 0) {
						msg.args.arg1 = res;
						send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					}
				}
				break;

				case MSG_DEV_CLOSE: {
					if(sockets[fd]) {
						usemdown(&sem);
						delete sockets[fd];
						usemup(&sem);
						sockets[fd] = NULL;
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
	close(sid);
	return 0;
}

int main(int argc,char **) {
	char echo1[] = "foo";
	char echo2[] = "barfoo";
	char echo3[] = "test";

	if(usemcrt(&sem,1) < 0)
		error("Unable to create semaphore");

	NIC ne2k("/dev/ne2k",argc == 1 ? IPv4Addr(192,168,2,110) : IPv4Addr(10,0,2,3));
	NIC lo("/dev/lo",IPv4Addr(127,0,0,1));

	Route::insert(IPv4Addr(192,168,2,0),IPv4Addr(255,255,255,0),IPv4Addr(),Route::FL_UP,&ne2k);
	Route::insert(IPv4Addr(),IPv4Addr(),IPv4Addr(192,168,2,1),Route::FL_UP | Route::FL_USE_GW,&ne2k);
	Route::insert(IPv4Addr(127,0,0,1),IPv4Addr(255,0,0,0),IPv4Addr(),Route::FL_UP,&lo);

	if(startthread(udpThread,NULL) < 0)
		error("Unable to start UDP thread");
	if(startthread(receiveThread,&ne2k) < 0)
		error("Unable to start receive thread");
	if(startthread(receiveThread,&lo) < 0)
		error("Unable to start receive thread");

#if 0
	usemdown(&sem);
	if(argc == 1) {
		if(ICMP::sendEcho(IPv4Addr(192,168,2,7),echo1,sizeof(echo1),0xFF81,1) < 0)
			printe("Unable to send echo packet");
		if(ICMP::sendEcho(IPv4Addr(192,168,2,1),echo2,sizeof(echo2),0xFF82,1) < 0)
			printe("Unable to send echo packet");
		if(ICMP::sendEcho(IPv4Addr(173,194,44,87),echo3,sizeof(echo3),0xFF83,1) < 0)
			printe("Unable to send echo packet");
		if(UDP::send(IPv4Addr(192,168,2,102),56123,7654,echo3,sizeof(echo3)) < 0)
			printe("Unable to send UDP packet");
	}
	else {
		if(ICMP::sendEcho(IPv4Addr(10,0,2,2),echo1,sizeof(echo1),0xFF88,1) < 0)
			printe("Unable to send echo packet");
		if(ICMP::sendEcho(IPv4Addr(10,0,2,2),echo2,sizeof(echo2),0xFF88,2) < 0)
			printe("Unable to send echo packet");
	}
	usemup(&sem);
#endif

	while(1)
		sleep(10000);
	return 0;
}
