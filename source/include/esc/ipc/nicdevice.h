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

#pragma once

#include <sys/common.h>
#include <esc/proto/pci.h>
#include <esc/proto/nic.h>
#include <esc/ipc/requestqueue.h>
#include <esc/ipc/clientdevice.h>
#include <stdlib.h>
#include <mutex>

namespace ipc {

class NICDriver {
public:
	struct Packet {
		Packet *next;
		size_t length;
		uint16_t data[];
	};

	explicit NICDriver() : _mutex(), _first(), _last() {
	}
	virtual ~NICDriver() {
	}

	virtual ipc::NIC::MAC mac() const = 0;
	virtual ulong mtu() const = 0;
	virtual ssize_t send(const void *packet,size_t size) = 0;

	Packet *fetch() {
		std::lock_guard<std::mutex> guard(_mutex);
		Packet *pkt = NULL;
		if(_first) {
			pkt = _first;
			_first = _first->next;
			if(!_first)
				_last = NULL;
		}
		return pkt;
	}

	void insert(Packet *pkt) {
		std::lock_guard<std::mutex> guard(_mutex);
		pkt->next = NULL;
		if(_last)
			_last->next = pkt;
		else
			_first = pkt;
		_last = pkt;
	}

private:
	std::mutex _mutex;
	Packet *_first;
	Packet *_last;
};

class NICDevice : public ClientDevice<> {
	struct EthernetHeader {
		ipc::NIC::MAC dst;
		ipc::NIC::MAC src;
		uint16_t type;
	};

public:
	explicit NICDevice(const char *path,mode_t mode,NICDriver *driver)
		: ClientDevice<>(path,mode,DEV_TYPE_CHAR,DEV_CANCEL | DEV_SHFILE | DEV_READ | DEV_WRITE),
		  _requests(std::make_memfun(this,&NICDevice::handleRead)), _mutex(), _driver(driver),
		  _tmpbuf(new char[_driver->mtu()]) {
		set(MSG_DEV_CANCEL,std::make_memfun(this,&NICDevice::cancel));
		set(MSG_FILE_READ,std::make_memfun(this,&NICDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&NICDevice::write));
		set(MSG_NIC_GETMAC,std::make_memfun(this,&NICDevice::getMac));
		set(MSG_NIC_GETMTU,std::make_memfun(this,&NICDevice::getMTU));
	}
	virtual ~NICDevice() {
		delete[] _tmpbuf;
	}

	NIC::MAC mac() const {
		return _driver->mac();
	}

	// called from drivers receive routine
	void checkPending() {
		std::lock_guard<std::mutex> guard(_mutex);
		_requests.handle();
	}

private:
	void cancel(IPCStream &is) {
		msgid_t mid;
		is >> mid;

		int res;
		// we answer write-requests always right away, so let the kernel just wait for the response
		if((mid & 0xFFFF) == MSG_FILE_WRITE)
			res = 1;
		else if((mid & 0xFFFF) != MSG_FILE_READ)
			res = -EINVAL;
		else {
			std::lock_guard<std::mutex> guard(_mutex);
			res = _requests.cancel(mid);
		}

		is << res << Reply();
	}

	void read(IPCStream &is) {
		Client *c = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;

		char *data = NULL;
		if(r.shmemoff != -1)
			data = c->shm() + r.shmemoff;

		if(!handleRead(is.fd(),is.msgid(),data,r.count)) {
			std::lock_guard<std::mutex> guard(_mutex);
			_requests.enqueue(Request(is.fd(),is.msgid(),data,r.count));
		}
	}

	void write(IPCStream &is) {
		char *data = _tmpbuf;
		FileWrite::Request r;
		is >> r;

		if(r.count > _driver->mtu()) {
			if(r.shmemoff == -1)
				is >> ReceiveData(NULL,0);
			is << FileWrite::Response(-EINVAL) << Reply();
			return;
		}

		if(r.shmemoff == -1)
			is >> ReceiveData(data,r.count);
		else
			data = (*this)[is.fd()]->shm() + r.shmemoff;

		// if it's for ourself, just forward it to our incoming packet list
		ssize_t res = -ENOMEM;
		EthernetHeader *eth = reinterpret_cast<EthernetHeader*>(data);
		if(eth->dst == _driver->mac()) {
			NICDriver::Packet *pkt = (NICDriver::Packet*)malloc(sizeof(NICDriver::Packet) + r.count);
			if(pkt) {
				pkt->length = r.count;
				memcpy(pkt->data,data,r.count);
				_driver->insert(pkt);
				checkPending();
				res = r.count;
			}
		}
		else
			res = _driver->send(data,r.count);

		is << FileWrite::Response(res) << Reply();
	}

	void getMac(IPCStream &is) {
		is << 0 << _driver->mac() << Reply();
	}

	void getMTU(IPCStream &is) {
		is << _driver->mtu() << Reply();
	}

	bool handleRead(int fd,msgid_t mid,char *data,size_t count) {
		NICDriver::Packet *pkt = _driver->fetch();
		if(!pkt)
			return false;

		ssize_t res = count >= pkt->length ? pkt->length : -ENOMEM;
		if(data && res > 0)
			memcpy(data,pkt->data,res);

		ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
		IPCStream is(fd,buffer,sizeof(buffer),mid);
		is << FileRead::Response(res) << Reply();
		if(!data && res > 0)
			is << ReplyData(pkt->data,res);

		free(pkt);
		return true;
	}

	RequestQueue _requests;
	std::mutex _mutex;
	NICDriver *_driver;
	char *_tmpbuf;
};

}
