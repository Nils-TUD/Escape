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
#include <mutex>

#include "ethernet.h"
#include "nic.h"
#include "arp.h"
#include "icmp.h"
#include "route.h"
#include "udp.h"

static std::mutex mutex;

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
	NICDevice *nic = reinterpret_cast<NICDevice*>(arg);
	while(1) {
		ssize_t res = nic->read(buffer,sizeof(buffer));
		if(res < 0) {
			printe("Reading packet failed");
			continue;
		}

		std::cout << "[" << nic->mac() << "] Received packet with " << res << " bytes" << std::endl;
		if((size_t)res >= sizeof(Ethernet<>)) {
			std::lock_guard<std::mutex> guard(mutex);
			Ethernet<> *packet = reinterpret_cast<Ethernet<>*>(buffer);
			std::cout << *packet << std::endl;
			ssize_t err = Ethernet<>::receive(*nic,packet,res);
			if(err < 0)
				printe("Invalid packet: %s",strerror(-err));
		}
		else
			printe("Ignoring packet of size %zd",res);
	}
	return 0;
}

class UDPDevice : public ipc::ClientDevice<UDPSocket> {
public:
	explicit UDPDevice(const char *path,mode_t mode)
		: ipc::ClientDevice<UDPSocket>(path,mode,DEV_TYPE_BLOCK,
			DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_WRITE | DEV_CLOSE) {
		set(MSG_FILE_OPEN,std::make_memfun(this,&UDPDevice::open));
		set(MSG_FILE_READ,std::make_memfun(this,&UDPDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&UDPDevice::write));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&UDPDevice::close),false);
	}

	void open(ipc::IPCStream &is) {
		char buffer[MAX_PATH_LEN];
		ipc::FileOpen::Request r(buffer,sizeof(buffer));
		is >> r;

		IPv4Addr ip;
		port_t port = 0;
		std::istringstream sip(r.path.str());
		sip >> ip >> port;

		add(is.fd(),new UDPSocket(is.fd(),ip,port));
		is << ipc::FileOpen::Response(0) << ipc::Send(ipc::FileOpen::Response::MID);
	}

	void read(ipc::IPCStream &is) {
		UDPSocket *sock = get(is.fd());
		ipc::FileRead::Request r;
		is >> r;

		ssize_t res;
		{
			char *data = NULL;
			if(r.shmemoff != -1)
				data = sock->shm() + r.shmemoff;

			std::lock_guard<std::mutex> guard(mutex);
			res = sock->receive(data,r.count);
		}

		if(res < 0)
			is << ipc::FileRead::Response(res) << ipc::Send(ipc::FileRead::Response::MID);
	}

	void write(ipc::IPCStream &is) {
		UDPSocket *sock = get(is.fd());
		char *data;
		ipc::FileWrite::Request r;
		is >> r;
		if(r.shmemoff == -1) {
			data = new char[r.count];
			is >> ipc::ReceiveData(data,r.count);
		}
		else
			data = sock->shm() + r.shmemoff;

		ssize_t res;
		{
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->sendto(NULL,data,r.count);
		}

		is << ipc::FileWrite::Response(res) << ipc::Send(ipc::FileWrite::Response::MID);
		if(r.shmemoff == -1)
			delete[] data;
	}

	void close(ipc::IPCStream &is) {
		std::lock_guard<std::mutex> guard(mutex);
		ipc::ClientDevice<UDPSocket>::close(is);
	}
};

static int udpThread(void*) {
	UDPDevice dev("/dev/udp",0777);
	dev.loop();
	return 0;
}

int main(int argc,char **) {
	char echo1[] = "foo";
	char echo2[] = "barfoo";
	char echo3[] = "test";

	NICDevice ne2k("/dev/ne2k",argc == 1 ? IPv4Addr(192,168,2,110) : IPv4Addr(10,0,2,3));
	NICDevice lo("/dev/lo",IPv4Addr(127,0,0,1));

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
