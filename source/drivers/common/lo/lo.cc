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
#include <ipc/proto/nic.h>
#include <ipc/clientdevice.h>
#include <ipc/requestqueue.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

class LoDevice : public ipc::ClientDevice<> {
	struct Packet {
		uint8_t *data;
		size_t size;
	};

public:
	static const ulong MTU;

	explicit LoDevice(const char *path,mode_t mode)
		: ipc::ClientDevice<>(path,mode,DEV_TYPE_CHAR,DEV_CANCEL | DEV_SHFILE | DEV_READ | DEV_WRITE),
		  _requests(std::make_memfun(this,&LoDevice::handleRead)), _packets() {
		set(MSG_DEV_CANCEL,std::make_memfun(this,&LoDevice::cancel));
		set(MSG_FILE_READ,std::make_memfun(this,&LoDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&LoDevice::write));
		set(MSG_NIC_GETMAC,std::make_memfun(this,&LoDevice::getMac));
		set(MSG_NIC_GETMTU,std::make_memfun(this,&LoDevice::getMTU));
	}

	void cancel(ipc::IPCStream &is) {
		msgid_t mid;
		is >> mid;

		int res;
		// we answer write-requests always right away, so let the kernel just wait for the response
		if((mid & 0xFFFF) == MSG_FILE_WRITE)
			res = 1;
		else if((mid & 0xFFFF) != MSG_FILE_READ)
			res = -EINVAL;
		else
			res = _requests.cancel(mid);

		is << res << ipc::Reply();
	}

	void read(ipc::IPCStream &is) {
		ipc::Client *c = (*this)[is.fd()];
		ipc::FileRead::Request r;
		is >> r;

		char *data = NULL;
		if(r.shmemoff != -1)
			data = c->shm() + r.shmemoff;

		if(_packets.size() > 0)
			handleRead(is.fd(),is.msgid(),data,r.count);
		else
			_requests.enqueue(ipc::Request(is.fd(),is.msgid(),data,r.count));
	}

	void write(ipc::IPCStream &is) {
		ipc::Client *c = (*this)[is.fd()];
		ipc::FileWrite::Request r;
		is >> r;

		if(r.count > MTU) {
			/* skip data message */
			is >> ipc::ReceiveData(NULL,0);
			is << ipc::FileWrite::Response(-EINVAL) << ipc::Reply();
			return;
		}

		Packet pkt;
		pkt.data = new uint8_t[r.count];
		pkt.size = r.count;

		if(r.shmemoff == -1)
			is >> ipc::ReceiveData(pkt.data,r.count);
		else
			memcpy(pkt.data,c->shm() + r.shmemoff,r.count);
		_packets.push_back(pkt);
		_requests.handle();

		is << ipc::FileWrite::Response(r.count) << ipc::Reply();
	}

	void getMac(ipc::IPCStream &is) {
		is << 0 << ipc::NIC::MAC() << ipc::Reply();
	}

	void getMTU(ipc::IPCStream &is) {
		is << MTU << ipc::Reply();
	}

private:
	bool handleRead(int fd,msgid_t mid,char *data,size_t count) {
		if(_packets.size() == 0)
			return false;

		Packet pkt = _packets.front();
		ssize_t res = count >= pkt.size ? pkt.size : -ENOMEM;
		if(data && res > 0)
			memcpy(data,pkt.data,res);

		ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
		ipc::IPCStream is(fd,buffer,sizeof(buffer),mid);
		is << ipc::FileRead::Response(res) << ipc::Reply();
		if(!data && res > 0)
			is << ipc::ReplyData(pkt.data,res);

		delete[] pkt.data;
		_packets.pop_back();
		return true;
	}

	ipc::RequestQueue _requests;
	std::list<Packet> _packets;
};

const ulong LoDevice::MTU = 64 * 1024;

int main(int argc,char **argv) {
	if(argc != 2)
		error("Usage: %s <device>\n",argv[0]);

	LoDevice dev(argv[1],0777);
	dev.loop();
	return EXIT_SUCCESS;
}
