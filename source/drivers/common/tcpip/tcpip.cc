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

#include <esc/ipc/filedev.h>
#include <esc/stream/istringstream.h>
#include <esc/dns.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/sync.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <usergroup/usergroup.h>
#include <mutex>
#include <stdio.h>
#include <vector>

#include "proto/arp.h"
#include "proto/ethernet.h"
#include "proto/icmp.h"
#include "proto/udp.h"
#include "socket/dgramsocket.h"
#include "socket/rawethersock.h"
#include "socket/rawipsock.h"
#include "socket/streamsocket.h"
#include "link.h"
#include "linkmng.h"
#include "packet.h"
#include "route.h"
#include "timeouts.h"

std::mutex mutex;

static int receiveThread(void *arg);

class SocketDevice : public esc::ClientDevice<Socket> {
public:
	explicit SocketDevice(const char *path,mode_t mode)
		: esc::ClientDevice<Socket>(path,mode,DEV_TYPE_BLOCK,
			DEV_OPEN | DEV_CANCEL | DEV_SHFILE | DEV_CREATSIBL | DEV_READ | DEV_WRITE | DEV_CLOSE) {
		set(MSG_FILE_OPEN,std::make_memfun(this,&SocketDevice::open));
		set(MSG_FILE_READ,std::make_memfun(this,&SocketDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&SocketDevice::write));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&SocketDevice::close),false);
		set(MSG_DEV_CANCEL,std::make_memfun(this,&SocketDevice::cancel));
		set(MSG_DEV_CREATSIBL,std::make_memfun(this,&SocketDevice::creatsibl));
		set(MSG_SOCK_CONNECT,std::make_memfun(this,&SocketDevice::connect));
		set(MSG_SOCK_BIND,std::make_memfun(this,&SocketDevice::bind));
		set(MSG_SOCK_LISTEN,std::make_memfun(this,&SocketDevice::listen));
		set(MSG_SOCK_RECVFROM,std::make_memfun(this,&SocketDevice::recvfrom));
		set(MSG_SOCK_SENDTO,std::make_memfun(this,&SocketDevice::sendto));
		set(MSG_SOCK_ABORT,std::make_memfun(this,&SocketDevice::abort));
	}

	void open(esc::IPCStream &is) {
		char buffer[MAX_PATH_LEN];
		esc::FileOpen::Request r(buffer,sizeof(buffer));
		is >> r;

		int type = 0, proto = 0;
		esc::IStringStream sip(r.path.str());
		sip >> type >> proto;

		int res = 0;
		{
			std::lock_guard<std::mutex> guard(mutex);
			switch(type) {
				case esc::Socket::SOCK_DGRAM:
					add(is.fd(),new DGramSocket(is.fd(),proto));
					break;
				case esc::Socket::SOCK_STREAM:
					add(is.fd(),new StreamSocket(is.fd(),proto));
					break;
				case esc::Socket::SOCK_RAW_ETHER:
					add(is.fd(),new RawEtherSocket(is.fd(),proto));
					break;
				case esc::Socket::SOCK_RAW_IP:
					add(is.fd(),new RawIPSocket(is.fd(),proto));
					break;
				default:
					res = -ENOTSUP;
					break;
			}
		}

		is << esc::FileOpen::Response::result(res) << esc::Reply();
	}

	void connect(esc::IPCStream &is) {
		Socket *sock = get(is.fd());
		esc::Socket::Addr sa;
		is >> sa;

		errcode_t res;
		{
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->connect(&sa,is.msgid());
		}
		if(res < 0)
			is << res << esc::Reply();
	}

	void bind(esc::IPCStream &is) {
		Socket *sock = get(is.fd());
		esc::Socket::Addr sa;
		is >> sa;

		errcode_t res;
		{
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->bind(&sa);
		}
		is << res << esc::Reply();
	}

	void listen(esc::IPCStream &is) {
		Socket *sock = get(is.fd());

		errcode_t res;
		{
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->listen();
		}
		is << res << esc::Reply();
	}

	void cancel(esc::IPCStream &is) {
		Socket *sock = get(is.fd());
		esc::DevCancel::Request r;
		is >> r;

		errcode_t res;
		if(r.msg != MSG_FILE_WRITE && r.msg != MSG_SOCK_SENDTO &&
				r.msg != MSG_FILE_READ && r.msg != MSG_SOCK_RECVFROM &&
				r.msg != MSG_DEV_CREATSIBL) {
			res = -EINVAL;
		}
		else {
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->cancel(r.mid);
		}

		is << esc::DevCancel::Response(res) << esc::Reply();
	}

	void creatsibl(esc::IPCStream &is) {
		Socket *sock = get(is.fd());
		esc::DevCreatSibl::Request r;
		is >> r;

		errcode_t res;
		{
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->accept(is.msgid(),r.nfd,this);
		}
		if(res < 0)
			is << esc::DevCreatSibl::Response(res) << esc::Reply();
	}

	void read(esc::IPCStream &is) {
		handleRead(is,false);
	}

	void recvfrom(esc::IPCStream &is) {
		handleRead(is,true);
	}

	void write(esc::IPCStream &is) {
		esc::FileWrite::Request r;
		is >> r;
		handleWrite(is,r,NULL);
	}

	void sendto(esc::IPCStream &is) {
		esc::FileWrite::Request r;
		esc::Socket::Addr sa;
		is >> r >> sa;
		handleWrite(is,r,&sa);
	}

	void abort(esc::IPCStream &is) {
		Socket *sock = get(is.fd());
		errcode_t res = sock->abort();
		is << res << esc::Reply();
	}

	void close(esc::IPCStream &is) {
		Socket *sock = get(is.fd());
		std::lock_guard<std::mutex> guard(mutex);
		// don't delete it; let the object itself decide when it is destroyed (for TCP)
		remove(is.fd(),false);
		sock->disconnect();
		Device::close(is);
	}

private:
	void handleRead(esc::IPCStream &is,bool needsSockAddr) {
		Socket *sock = get(is.fd());
		esc::FileRead::Request r;
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
			is << esc::FileRead::Response::error(res) << esc::Reply();
	}

	void handleWrite(esc::IPCStream &is,esc::FileWrite::Request &r,const esc::Socket::Addr *sa) {
		Socket *sock = get(is.fd());

		// TODO check offset!
		esc::DataBuf buf(r.count,sock->shm(),r.shmemoff);
		if(r.shmemoff == -1)
			is >> esc::ReceiveData(buf.data(),r.count);

		ssize_t res;
		{
			std::lock_guard<std::mutex> guard(mutex);
			res = sock->sendto(is.msgid(),sa,buf.data(),r.count);
		}

		if(res != 0)
			is << esc::FileWrite::Response::result(res) << esc::Reply();
	}
};

class NetDevice : public esc::Device {
public:
	explicit NetDevice(const char *path,mode_t mode)
		: esc::Device(path,mode,DEV_TYPE_SERVICE,0) {
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

	void linkAdd(esc::IPCStream &is) {
		esc::CStringBuf<Link::NAME_LEN> name;
		esc::CStringBuf<MAX_PATH_LEN> path;
		is >> name >> path;

		std::lock_guard<std::mutex> guard(mutex);
		errcode_t res = LinkMng::add(name.str(),path.str());
		if(res == 0) {
			std::shared_ptr<Link> link = LinkMng::getByName(name.str());
			std::shared_ptr<Link> *linkcpy = new std::shared_ptr<Link>(link);
			if((res = startthread(receiveThread,linkcpy)) < 0) {
				LinkMng::rem(name.str());
				delete linkcpy;
			}
		}
		is << res << esc::Reply();
	}

	void linkRem(esc::IPCStream &is) {
		esc::CStringBuf<Link::NAME_LEN> name;
		is >> name;

		std::lock_guard<std::mutex> guard(mutex);
		errcode_t res = LinkMng::rem(name.str());
		is << res << esc::Reply();
	}

	void linkConfig(esc::IPCStream &is) {
		esc::CStringBuf<Link::NAME_LEN> name;
		esc::Net::IPv4Addr ip,netmask;
		esc::Net::Status status;
		is >> name >> ip >> netmask >> status;

		std::lock_guard<std::mutex> guard(mutex);
		errcode_t res = 0;
		std::shared_ptr<Link> l = LinkMng::getByName(name.str());
		std::shared_ptr<Link> other;
		if(!l)
			res = -ENOTFOUND;
		else if((other = LinkMng::getByIp(ip)) && other != l)
			res = -EEXIST;
		else if(ip.value() != 0 && !ip.isHost(netmask))
			res = -EINVAL;
		else if(!netmask.isNetmask() || status == esc::Net::KILLED)
			res = -EINVAL;
		else {
			l->ip(ip);
			l->subnetMask(netmask);
			l->status(status);
		}
		is << res << esc::Reply();
	}

	void linkMAC(esc::IPCStream &is) {
		esc::CStringBuf<Link::NAME_LEN> name;
		is >> name;

		std::lock_guard<std::mutex> guard(mutex);
		std::shared_ptr<Link> link = LinkMng::getByName(name.str());
		if(!link)
			is << esc::ValueResponse<esc::NIC::MAC>::error(-ENOTFOUND) << esc::Reply();
		else
			is << esc::ValueResponse<esc::NIC::MAC>::success(link->mac()) << esc::Reply();
	}

	void routeAdd(esc::IPCStream &is) {
		esc::CStringBuf<Link::NAME_LEN> link;
		esc::Net::IPv4Addr ip,gw,netmask;
		is >> link >> ip >> gw >> netmask;

		std::lock_guard<std::mutex> guard(mutex);
		errcode_t res = 0;
		std::shared_ptr<Link> l = LinkMng::getByName(link.str());
		if(!l)
			res = -ENOTFOUND;
		else {
			uint flags = esc::Net::FL_UP;
			if(gw.value() != 0)
				flags |= esc::Net::FL_USE_GW;
			Route::insert(ip,netmask,gw,flags,l);
		}
		is << res << esc::Reply();
	}

	void routeRem(esc::IPCStream &is) {
		esc::Net::IPv4Addr ip;
		is >> ip;

		std::lock_guard<std::mutex> guard(mutex);
		errcode_t res = Route::remove(ip);
		is << res << esc::Reply();
	}

	void routeConfig(esc::IPCStream &is) {
		esc::Net::IPv4Addr ip;
		esc::Net::Status status;
		is >> ip >> status;

		std::lock_guard<std::mutex> guard(mutex);
		errcode_t res = Route::setStatus(ip,status);
		is << res << esc::Reply();
	}

	void routeGet(esc::IPCStream &is) {
		esc::Net::IPv4Addr ip;
		is >> ip;

		std::lock_guard<std::mutex> guard(mutex);
		Route r = Route::find(ip);
		if(!r.valid())
			is << errcode_t(-ENETUNREACH) << esc::Reply();
		else {
			is << errcode_t(0);
			is << (r.flags & esc::Net::FL_USE_GW ? r.gateway : r.dest);
			is << esc::CString(r.link->name().c_str(),r.link->name().length());
			is << esc::Reply();
		}
	}

	void arpAdd(esc::IPCStream &is) {
		esc::Net::IPv4Addr ip;
		is >> ip;

		std::lock_guard<std::mutex> guard(mutex);
		errcode_t res = 0;
		Route route = Route::find(ip);
		if(!route.valid())
			res = -ENOTFOUND;
		else
			ARP::requestMAC(route.link,ip);
		is << res << esc::Reply();
	}

	void arpRem(esc::IPCStream &is) {
		esc::Net::IPv4Addr ip;
		is >> ip;

		std::lock_guard<std::mutex> guard(mutex);
		errcode_t res = ARP::remove(ip);
		is << res << esc::Reply();
	}
};

class LinksFileDevice : public esc::FileDevice {
public:
	explicit LinksFileDevice(const char *path,mode_t mode)
		: esc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		esc::OStringStream os;
		std::lock_guard<std::mutex> guard(mutex);
		LinkMng::print(os);
		return os.str();
	}
};

class RoutesFileDevice : public esc::FileDevice {
public:
	explicit RoutesFileDevice(const char *path,mode_t mode)
		: esc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		esc::OStringStream os;
		std::lock_guard<std::mutex> guard(mutex);
		Route::print(os);
		return os.str();
	}
};

class ARPFileDevice : public esc::FileDevice {
public:
	explicit ARPFileDevice(const char *path,mode_t mode)
		: esc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		esc::OStringStream os;
		std::lock_guard<std::mutex> guard(mutex);
		ARP::print(os);
		return os.str();
	}
};

class SocketsFileDevice : public esc::FileDevice {
public:
	explicit SocketsFileDevice(const char *path,mode_t mode)
		: esc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		esc::OStringStream os;
		std::lock_guard<std::mutex> guard(mutex);
		TCP::printSockets(os);
		UDP::printSockets(os);
		return os.str();
	}
};

static int receiveThread(void *arg) {
	std::shared_ptr<Link> *linkptr = reinterpret_cast<std::shared_ptr<Link>*>(arg);
	const std::shared_ptr<Link> link = *linkptr;
	uint8_t *buffer = reinterpret_cast<uint8_t*>(link->sharedmem());
	while(link->status() != esc::Net::KILLED) {
		ssize_t res = link->read(buffer,link->mtu());
		if(res < 0) {
			printe("Reading packet failed");
			break;
		}

		if((size_t)res >= sizeof(Ethernet<>)) {
			std::lock_guard<std::mutex> guard(mutex);
			Packet pkt(buffer,res);
			ssize_t err = Ethernet<>::receive(link,pkt);
			if(err < 0)
				std::cerr << "Ignored packet of size " << res << ": " << strerror(err) << "\n";
		}
		else
			printe("Ignoring packet of size %zd",res);
	}
	LinkMng::rem(link->name());
	delete linkptr;
	return 0;
}

static int linksFileThread(void*) {
	LinksFileDevice dev("/sys/net/links",0444);
	dev.loop();
	return 0;
}

static int routesFileThread(void*) {
	RoutesFileDevice dev("/sys/net/routes",0444);
	dev.loop();
	return 0;
}

static int arpFileThread(void*) {
	ARPFileDevice dev("/sys/net/arp",0444);
	dev.loop();
	return 0;
}

static int socketsFileThread(void*) {
	SocketsFileDevice dev("/sys/net/sockets",0444);
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
	const char *resolvconf = esc::DNS::getResolveFile();
	print("Creating empty %s",resolvconf);
	int fd = creat(resolvconf,0660);
	if(fd < 0) {
		printe("Unable to create %s",resolvconf);
		return;
	}
	close(fd);

	// chown it to the network-group
	size_t groupCount;
	sNamedItem *groups = usergroup_parse(GROUPS_PATH,&groupCount);
	sNamedItem *network = usergroup_getByName(groups,"network");
	if(!network || chown(resolvconf,0,network->id) < 0)
		printe("Unable to chmod %s",resolvconf);
	usergroup_free(groups);
}

int main() {
	srand(rdtsc());

	print("Creating /sys/net");
	if(mkdir("/sys/net",DIR_DEF_MODE) < 0)
		printe("Unable to create /sys/net");
	createResolvConf();

	if(startthread(socketThread,NULL) < 0)
		error("Unable to start socket thread");
	if(startthread(linksFileThread,NULL) < 0)
		error("Unable to start links-file thread");
	if(startthread(routesFileThread,NULL) < 0)
		error("Unable to start routes-file thread");
	if(startthread(arpFileThread,NULL) < 0)
		error("Unable to start arp-file thread");
	if(startthread(socketsFileThread,NULL) < 0)
		error("Unable to start sockets-file thread");
	if(startthread(Timeouts::thread,NULL) < 0)
		error("Unable to start timeout thread");

	NetDevice netdev("/dev/tcpip",0111);
	netdev.loop();
	return 0;
}
