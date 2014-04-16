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
#include <usergroup/group.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <mutex>
#include <vector>
#include <dns.h>

#include "proto/ethernet.h"
#include "proto/arp.h"
#include "proto/icmp.h"
#include "proto/udp.h"
#include "socket/dgramsocket.h"
#include "socket/rawipsock.h"
#include "socket/rawethersock.h"
#include "link.h"
#include "linkmng.h"
#include "route.h"
#include "packet.h"

static std::mutex mutex;

static int receiveThread(void *arg);

class SocketDevice : public ipc::ClientDevice<Socket> {
public:
	explicit SocketDevice(const char *path,mode_t mode)
		: ipc::ClientDevice<Socket>(path,mode,DEV_TYPE_BLOCK,
			DEV_OPEN | DEV_CANCEL | DEV_SHFILE | DEV_READ | DEV_WRITE | DEV_CLOSE) {
		set(MSG_FILE_OPEN,std::make_memfun(this,&SocketDevice::open));
		set(MSG_FILE_READ,std::make_memfun(this,&SocketDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&SocketDevice::write));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&SocketDevice::close),false);
		set(MSG_DEV_CANCEL,std::make_memfun(this,&SocketDevice::cancel));
		set(MSG_SOCK_BIND,std::make_memfun(this,&SocketDevice::bind));
		set(MSG_SOCK_RECVFROM,std::make_memfun(this,&SocketDevice::recvfrom));
		set(MSG_SOCK_SENDTO,std::make_memfun(this,&SocketDevice::sendto));
	}

	void open(ipc::IPCStream &is) {
		char buffer[MAX_PATH_LEN];
		ipc::FileOpen::Request r(buffer,sizeof(buffer));
		is >> r;

		int type = 0, proto = 0;
		std::istringstream sip(r.path.str());
		sip >> type >> proto;

		int res = 0;
		if(type == ipc::Socket::SOCK_DGRAM)
			add(is.fd(),new DGramSocket(is.fd(),proto));
		else if(type == ipc::Socket::SOCK_RAW_ETHER)
			add(is.fd(),new RawEtherSocket(is.fd(),proto));
		else if(type == ipc::Socket::SOCK_RAW_IP)
			add(is.fd(),new RawIPSocket(is.fd(),proto));
		else
			res = -ENOTSUP;

		is << ipc::FileOpen::Response(res) << ipc::Reply();
	}

	void bind(ipc::IPCStream &is) {
		Socket *sock = get(is.fd());
		ipc::Socket::Addr sa;
		is >> sa;

		int res;
		{
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->bind(&sa);
		}
		is << res << ipc::Reply();
	}

	void cancel(ipc::IPCStream &is) {
		Socket *sock = get(is.fd());
		msgid_t mid;
		is >> mid;

		int res;
		// we answer write-requests always right away, so let the kernel just wait for the response
		if((mid & 0xFFFF) == MSG_FILE_WRITE)
			res = 0;
		else if((mid & 0xFFFF) != MSG_FILE_READ && (mid & 0xFFFF) != MSG_SOCK_RECVFROM)
			res = -EINVAL;
		else {
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->cancel(mid);
		}

		is << res << ipc::Reply();
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
			res = sock->recvfrom(is.msgid(),needsSockAddr,data,r.count);
		}

		if(res < 0)
			is << ipc::FileRead::Response(res) << ipc::Reply();
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

		is << ipc::FileWrite::Response(res) << ipc::Reply();
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
		set(MSG_NET_ROUTE_GET,std::make_memfun(this,&NetDevice::routeGet));
		set(MSG_NET_ARP_ADD,std::make_memfun(this,&NetDevice::arpAdd));
		set(MSG_NET_ARP_REM,std::make_memfun(this,&NetDevice::arpRem));
	}

	void linkAdd(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> name;
		ipc::CStringBuf<MAX_PATH_LEN> path;
		is >> name >> path;

		std::lock_guard<std::mutex> guard(mutex);
		int res = LinkMng::add(name.str(),path.str());
		if(res == 0) {
			Link *link = LinkMng::getByName(name.str());
			if((res = startthread(receiveThread,link)) < 0) {
				LinkMng::rem(name.str());
				delete link;
			}
		}
		is << res << ipc::Reply();
	}

	void linkRem(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> name;
		is >> name;

		std::lock_guard<std::mutex> guard(mutex);
		int res = LinkMng::rem(name.str());
		is << res << ipc::Reply();
	}

	void linkConfig(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> name;
		ipc::Net::IPv4Addr ip,netmask;
		ipc::Net::Status status;
		is >> name >> ip >> netmask >> status;

		std::lock_guard<std::mutex> guard(mutex);
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
		is << res << ipc::Reply();
	}

	void linkMAC(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> name;
		is >> name;

		std::lock_guard<std::mutex> guard(mutex);
		Link *link = LinkMng::getByName(name.str());
		if(!link)
			is << -ENOENT << ipc::Reply();
		else
			is << 0 << link->mac() << ipc::Reply();
	}

	void routeAdd(ipc::IPCStream &is) {
		ipc::CStringBuf<Link::NAME_LEN> link;
		ipc::Net::IPv4Addr ip,gw,netmask;
		is >> link >> ip >> gw >> netmask;

		std::lock_guard<std::mutex> guard(mutex);
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
		is << res << ipc::Reply();
	}

	void routeRem(ipc::IPCStream &is) {
		ipc::Net::IPv4Addr ip;
		is >> ip;

		std::lock_guard<std::mutex> guard(mutex);
		int res = Route::remove(ip);
		is << res << ipc::Reply();
	}

	void routeConfig(ipc::IPCStream &is) {
		ipc::Net::IPv4Addr ip;
		ipc::Net::Status status;
		is >> ip >> status;

		std::lock_guard<std::mutex> guard(mutex);
		int res = Route::setStatus(ip,status);
		is << res << ipc::Reply();
	}

	void routeGet(ipc::IPCStream &is) {
		ipc::Net::IPv4Addr ip;
		is >> ip;

		std::lock_guard<std::mutex> guard(mutex);
		const Route *r = Route::find(ip);
		if(!r)
			is << -ENETUNREACH << ipc::Reply();
		else {
			is << 0;
			is << (r->flags & ipc::Net::FL_USE_GW ? r->gateway : r->dest);
			is << ipc::CString(r->link->name().c_str(),r->link->name().length());
			is << ipc::Reply();
		}
	}

	void arpAdd(ipc::IPCStream &is) {
		ipc::Net::IPv4Addr ip;
		is >> ip;

		std::lock_guard<std::mutex> guard(mutex);
		int res = 0;
		const Route *route = Route::find(ip);
		if(!route)
			res = -ENOENT;
		else
			ARP::requestMAC(*route->link,ip);
		is << res << ipc::Reply();
	}

	void arpRem(ipc::IPCStream &is) {
		ipc::Net::IPv4Addr ip;
		is >> ip;

		std::lock_guard<std::mutex> guard(mutex);
		int res = ARP::remove(ip);
		is << res << ipc::Reply();
	}
};

class LinksFileDevice : public ipc::FileDevice {
public:
	explicit LinksFileDevice(const char *path,mode_t mode)
		: ipc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		std::ostringstream os;
		std::lock_guard<std::mutex> guard(mutex);
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
		std::lock_guard<std::mutex> guard(mutex);
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
		std::lock_guard<std::mutex> guard(mutex);
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

		if((size_t)res >= sizeof(Ethernet<>)) {
			std::lock_guard<std::mutex> guard(mutex);
			Packet pkt(buffer,res);
			//std::cout << "Got packet of " << res << " bytes:\n";
			//std::cout << *reinterpret_cast<Ethernet<>*>(buffer) << std::endl;
			ssize_t err = Ethernet<>::receive(*link,pkt);
			if(err < 0)
				fprintf(stderr,"Invalid packet: %s",strerror(err));
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

static void createResolvConf() {
	// create file
	const char *resolvconf = std::DNS::getResolveFile();
	print("Creating empty %s",resolvconf);
	int fd = create(resolvconf,IO_WRITE | IO_CREATE,0660);
	if(fd < 0) {
		printe("Unable to create %s",resolvconf);
		return;
	}
	close(fd);

	// chown it to the network-group
	size_t groupCount;
	sGroup *groups = group_parseFromFile(GROUPS_PATH,&groupCount);
	sGroup *network = group_getByName(groups,"network");
	if(!network || chown(resolvconf,0,network->gid) < 0)
		printe("Unable to chmod %s",resolvconf);
	group_free(groups);
}

int main() {
	print("Creating /system/net");
	if(mkdir("/system/net") < 0)
		printe("Unable to create /system/net");
	createResolvConf();

	if(startthread(socketThread,NULL) < 0)
		error("Unable to start socket thread");
	if(startthread(linksFileThread,NULL) < 0)
		error("Unable to start links-file thread");
	if(startthread(routesFileThread,NULL) < 0)
		error("Unable to start routes-file thread");
	if(startthread(arpFileThread,NULL) < 0)
		error("Unable to start arp-file thread");

	NetDevice netdev("/dev/tcpip",0111);
	netdev.loop();
	return 0;
}
