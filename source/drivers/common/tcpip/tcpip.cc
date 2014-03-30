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
#include <esc/sync.h>
#include <stdio.h>
#include <iostream>

#include "ethernet.h"
#include "nic.h"
#include "arp.h"
#include "icmp.h"
#include "route.h"

static tUserSem sem;

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

		std::cout << "Received packet with " << res << " bytes" << std::endl;
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

int main(int argc,char **) {
	char echo1[] = "foo";
	char echo2[] = "barfoo";
	char echo3[] = "test";

	if(usemcrt(&sem,1) < 0)
		error("Unable to create semaphore");

	NIC nic("/dev/ne2k",argc == 1 ? IPv4Addr(192,168,2,110) : IPv4Addr(10,0,2,3));

	Route::insert(IPv4Addr(192,168,2,0),IPv4Addr(255,255,255,0),IPv4Addr(),Route::FL_UP,&nic);
	Route::insert(IPv4Addr(),IPv4Addr(),IPv4Addr(192,168,2,1),Route::FL_UP | Route::FL_USE_GW,&nic);

	if(startthread(receiveThread,&nic) < 0)
		error("Unable to start receive thread");

	usemdown(&sem);
	if(argc == 1) {
		if(ICMP::sendEcho(IPv4Addr(192,168,2,7),echo1,sizeof(echo1),0xFF81,1) < 0)
			printe("Unable to send echo packet");
		if(ICMP::sendEcho(IPv4Addr(192,168,2,1),echo2,sizeof(echo2),0xFF82,1) < 0)
			printe("Unable to send echo packet");
		if(ICMP::sendEcho(IPv4Addr(173,194,44,87),echo3,sizeof(echo3),0xFF83,1) < 0)
			printe("Unable to send echo packet");
	}
	else {
		if(ICMP::sendEcho(IPv4Addr(10,0,2,2),echo1,sizeof(echo1),0xFF88,1) < 0)
			printe("Unable to send echo packet");
		if(ICMP::sendEcho(IPv4Addr(10,0,2,2),echo2,sizeof(echo2),0xFF88,2) < 0)
			printe("Unable to send echo packet");
	}
	usemup(&sem);

	while(1)
		sleep(10000);
	return 0;
}
