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
#include <ipc/filedev.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <mutex>
#include <vector>

#include "proto/ethernet.h"
#include "proto/arp.h"
#include "proto/icmp.h"
#include "proto/udp.h"
#include "link.h"
#include "linkmng.h"
#include "route.h"

static std::mutex mutex;

static int receiveThread(void *arg);

class SocketDevice : public ipc::ClientDevice<Socket> {
public:
	explicit SocketDevice(const char *path,mode_t mode)
		: ipc::ClientDevice<Socket>(path,mode,DEV_TYPE_BLOCK,
			DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_WRITE | DEV_CLOSE) {
		set(MSG_FILE_OPEN,std::make_memfun(this,&SocketDevice::open));
		set(MSG_FILE_READ,std::make_memfun(this,&SocketDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&SocketDevice::write));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&SocketDevice::close),false);
		set(MSG_SOCK_BIND,std::make_memfun(this,&SocketDevice::bind));
		set(MSG_SOCK_RECVFROM,std::make_memfun(this,&SocketDevice::recvfrom));
		set(MSG_SOCK_SENDTO,std::make_memfun(this,&SocketDevice::sendto));
	}

	void open(ipc::IPCStream &is) {
		char buffer[MAX_PATH_LEN];
		ipc::FileOpen::Request r(buffer,sizeof(buffer));
		is >> r;

		int domain, type, proto;
		std::istringstream sip(r.path.str());
		sip >> domain >> type >> proto;

		int res = 0;
		if(domain != ipc::Socket::AF_INET)
			res = -ENOTSUP;
		else {
			if(proto == ipc::Socket::PROTO_UDP)
				add(is.fd(),new UDPSocket(is.fd(),type));
			else
				res = -ENOTSUP;
		}

		is << ipc::FileOpen::Response(res) << ipc::Send(ipc::FileOpen::Response::MID);
	}

	void bind(ipc::IPCStream &is) {
		Socket *sock = get(is.fd());
		ipc::Socket::Addr sa;
		is >> sa;

		int res = sock->bind(&sa);
		is << res << ipc::Send(MSG_DEF_RESPONSE);
	}

	void read(ipc::IPCStream &is) {
		handleRead(is,false);
	}

	void recvfrom(ipc::IPCStream &is) {
		handleRead(is,true);
	}

	void write(ipc::IPCStream &is) {
		ipc::FileWrite::Request r;
		is >> r;
		handleWrite(is,r,NULL);
	}

	void sendto(ipc::IPCStream &is) {
		ipc::FileWrite::Request r;
		ipc::Socket::Addr sa;
		is >> r >> sa;
		handleWrite(is,r,&sa);
	}

	void close(ipc::IPCStream &is) {
		std::lock_guard<std::mutex> guard(mutex);
		ipc::ClientDevice<Socket>::close(is);
	}

private:
	void handleRead(ipc::IPCStream &is,bool needsSockAddr) {
		Socket *sock = get(is.fd());
		ipc::FileRead::Request r;
		is >> r;

		ssize_t res;
		{
			// TODO check offset!
			char *data = NULL;
			if(r.shmemoff != -1)
				data = sock->shm() + r.shmemoff;

			std::lock_guard<std::mutex> guard(mutex);
			res = sock->recvfrom(needsSockAddr,data,r.count);
		}

		if(res < 0)
			is << ipc::FileRead::Response(res) << ipc::Send(ipc::FileRead::Response::MID);
	}

	void handleWrite(ipc::IPCStream &is,ipc::FileWrite::Request &r,const ipc::Socket::Addr *sa) {
		Socket *sock = get(is.fd());
		char *data;
		if(r.shmemoff == -1) {
			data = new char[r.count];
			is >> ipc::ReceiveData(data,r.count);
		}
		// TODO check offset!
		else
			data = sock->shm() + r.shmemoff;

		ssize_t res;
		{
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->sendto(sa,data,r.count);
		}

		is << ipc::FileWrite::Response(res) << ipc::Send(ipc::FileWrite::Response::MID);
		if(r.shmemoff == -1)
			delete[] data;
	}
};

class NetDevice : public ipc::Device {
public:
	explicit NetDevice(const char *path,mode_t mode)
		: ipc::Device(path,mode,DEV_TYPE_SERVICE,0) {
		set(MSG_NET_LINK_ADD,std::make_memfun(this,&NetDevice::linkAdd));
		set(MSG_NET_LINK_REM,std::make_memfun(this,&NetDevice::linkRem));
		set(MSG_NET_LINK_CONFIG,std::make_memfun(this,&NetDevice::linkConfig));
		set(MSG_NET_LINK_MAC,std::make_memfun(this,&NetDevice::linkMAC));
		set(MSG_NET_ROUTE_ADD,std::make_memfun(this,&NetDevice::routeAdd));
		set(MSG_NET_ROUTE_REM,std::make_memfun(this,&NetDevice::routeRem));
		set(MSG_NET_ROUTE_CONFIG,std::make_memfun(this,&NetDevice::routeConfig));
		set(MSG_NET_ARP_ADD,std::make_memfun(this,&NetDevice::arpAdd));
		set(MSG_NET_ARP_REM,std::make_memfun(this,&NetDevice::arpRem));
	}

	void linkAdd(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> name;
		ipc::CStringBuf<MAX_PATH_LEN> path;
		is >> name >> path;

		int res = LinkMng::add(name.str(),path.str());
		if(res == 0) {
			Link *link = LinkMng::getByName(name.str());
			if((res = startthread(receiveThread,link)) < 0) {
				LinkMng::rem(name.str());
				delete link;
			}
		}
		is << res << ipc::Send(MSG_DEF_RESPONSE);
	}

	void linkRem(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> name;
		is >> name;

		int res = LinkMng::rem(name.str());
		is << res << ipc::Send(MSG_DEF_RESPONSE);
	}

	void linkConfig(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> name;
		ipc::Net::IPv4Addr ip,netmask;
		ipc::Net::Status status;
		is >> name >> ip >> netmask >> status;

		int res = 0;
		Link *l = LinkMng::getByName(name.str());
		Link *other;
		if(l == NULL)
			res = -ENOENT;
		else if((other = LinkMng::getByIp(ip)) && other != l)
			res = -EEXIST;
		else if(ip.value() != 0 && !ip.isHost(netmask))
			res = -EINVAL;
		else if(!netmask.isNetmask() || status == ipc::Net::KILLED)
			res = -EINVAL;
		else {
			l->ip(ip);
			l->subnetMask(netmask);
			l->status(status);
		}
		is << res << ipc::Send(MSG_DEF_RESPONSE);
	}

	void linkMAC(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> name;
		is >> name;

		Link *link = LinkMng::getByName(name.str());
		if(!link)
			is << -ENOENT << ipc::Send(MSG_DEF_RESPONSE);
		else
			is << 0 << link->mac() << ipc::Send(MSG_DEF_RESPONSE);
	}

	void routeAdd(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> link;
		ipc::Net::IPv4Addr ip,gw,netmask;
		is >> link >> ip >> gw >> netmask;

		int res = 0;
		Link *l = LinkMng::getByName(link.str());
		if(l == NULL)
			res = -ENOENT;
		else {
			uint flags = ipc::Net::FL_UP;
			if(gw.value() != 0)
				flags |= ipc::Net::FL_USE_GW;
			Route::insert(ip,netmask,gw,flags,l);
		}
		is << res << ipc::Send(MSG_DEF_RESPONSE);
	}

	void routeRem(ipc::IPCStream &is) {
		ipc::Net::IPv4Addr ip;
		is >> ip;

		int res = Route::remove(ip);
		is << res << ipc::Send(MSG_DEF_RESPONSE);
	}

	void routeConfig(ipc::IPCStream &is) {
		ipc::Net::IPv4Addr ip;
		ipc::Net::Status status;
		is >> ip >> status;

		int res = Route::setStatus(ip,status);
		is << res << ipc::Send(MSG_DEF_RESPONSE);
	}

	void arpAdd(ipc::IPCStream &is) {
		ipc::Net::IPv4Addr ip;
		is >> ip;

		int res = 0;
		const Route *route = Route::find(ip);
		if(!route)
			res = -ENOENT;
		else
			ARP::requestMAC(*route->link,ip);
		is << res << ipc::Send(MSG_DEF_RESPONSE);
	}

	void arpRem(ipc::IPCStream &is) {
		ipc::Net::IPv4Addr ip;
		is >> ip;

		int res = ARP::remove(ip);
		is << res << ipc::Send(MSG_DEF_RESPONSE);
	}
};

class LinksFileDevice : public ipc::FileDevice {
public:
	explicit LinksFileDevice(const char *path,mode_t mode)
		: ipc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		std::ostringstream os;
		LinkMng::print(os);
		return os.str();
	}
};

class RoutesFileDevice : public ipc::FileDevice {
public:
	explicit RoutesFileDevice(const char *path,mode_t mode)
		: ipc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		std::ostringstream os;
		Route::print(os);
		return os.str();
	}
};

class ARPFileDevice : public ipc::FileDevice {
public:
	explicit ARPFileDevice(const char *path,mode_t mode)
		: ipc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		std::ostringstream os;
		ARP::print(os);
		return os.str();
	}
};

static int receiveThread(void *arg) {
	static uint8_t buffer[4096];
	Link *link = reinterpret_cast<Link*>(arg);
	while(link->status() != ipc::Net::KILLED) {
		ssize_t res = link->read(buffer,sizeof(buffer));
		if(res < 0) {
			printe("Reading packet failed");
			break;
		}

		std::cout << "[" << link->mac() << "] Received packet with " << res << " bytes" << std::endl;
		if((size_t)res >= sizeof(Ethernet<>)) {
			std::lock_guard<std::mutex> guard(mutex);
			Ethernet<> *packet = reinterpret_cast<Ethernet<>*>(buffer);
			std::cout << *packet << std::endl;
			ssize_t err = Ethernet<>::receive(*link,packet,res);
			if(err < 0)
				printe("Invalid packet: %s",strerror(-err));
		}
		else
			printe("Ignoring packet of size %zd",res);
	}
	delete link;
	return 0;
}

static int linksFileThread(void*) {
	LinksFileDevice dev("/system/net/links",0444);
	dev.loop();
	return 0;
}

static int routesFileThread(void*) {
	RoutesFileDevice dev("/system/net/routes",0444);
	dev.loop();
	return 0;
}

static int arpFileThread(void*) {
	ARPFileDevice dev("/system/net/arp",0444);
	dev.loop();
	return 0;
}

static int socketThread(void*) {
	SocketDevice dev("/dev/socket",0777);
	dev.loop();
	return 0;
}

int main() {
	if(mkdir("/system/net") < 0)
		printe("Unable to create /system/net");

	if(startthread(socketThread,NULL) < 0)
		error("Unable to start socket thread");
	if(startthread(linksFileThread,NULL) < 0)
		error("Unable to start links-file thread");
	if(startthread(routesFileThread,NULL) < 0)
		error("Unable to start routes-file thread");
	if(startthread(arpFileThread,NULL) < 0)
		error("Unable to start arp-file thread");

#if 0
	char echo1[] = "foo";
	char echo2[] = "barfoo";
	char echo3[] = "test";

	Link ne2k("/dev/ne2k",argc == 1 ? ipc::Net::IPv4Addr(192,168,2,110) : ipc::Net::IPv4Addr(10,0,2,3));
	Link lo("/dev/lo",ipc::Net::IPv4Addr(127,0,0,1));

	Route::insert(
		ipc::Net::IPv4Addr(192,168,2,0),ipc::Net::IPv4Addr(255,255,255,0),ipc::Net::IPv4Addr(),
		Route::FL_UP,&ne2k
	);
	Route::insert(
		ipc::Net::IPv4Addr(),ipc::Net::IPv4Addr(),ipc::Net::IPv4Addr(192,168,2,1),
		Route::FL_UP | Route::FL_USE_GW,&ne2k
	);
	Route::insert(
		ipc::Net::IPv4Addr(127,0,0,1),ipc::Net::IPv4Addr(255,0,0,0),ipc::Net::IPv4Addr(),
		Route::FL_UP,&lo
	);

	if(startthread(receiveThread,&ne2k) < 0)
		error("Unable to start receive thread");
	if(startthread(receiveThread,&lo) < 0)
		error("Unable to start receive thread");

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

	NetDevice netdev("/dev/tcpip",0111);
	netdev.loop();
	return 0;
}
