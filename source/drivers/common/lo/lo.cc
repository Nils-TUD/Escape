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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <list>

class LoDevice : public ipc::ClientDevice<> {
	struct Packet {
		uint8_t *data;
		size_t size;
	};

public:
	explicit LoDevice(const char *path,mode_t mode)
		: ipc::ClientDevice<>(path,mode,DEV_TYPE_CHAR,DEV_SHFILE | DEV_READ | DEV_WRITE), _packets() {
		set(MSG_FILE_READ,std::make_memfun(this,&LoDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&LoDevice::write));
		set(MSG_NIC_GETMAC,std::make_memfun(this,&LoDevice::getMac));
	}

	void read(ipc::IPCStream &is) {
		ipc::Client *c = get(is.fd());
		ipc::FileRead::Request r;
		is >> r;

		char *data = NULL;
		if(r.shmemoff != -1)
			data = c->shm() + r.shmemoff;

		assert(_packets.size() > 0);

		Packet pkt = _packets.front();
		size_t res = r.count >= pkt.size ? pkt.size : -ENOMEM;
		if(r.shmemoff != -1 && res)
			memcpy(data,pkt.data,res);

		is << ipc::FileRead::Response(res) << ipc::Send(ipc::FileRead::Response::MID);
		if(r.shmemoff == -1 && res)
			is << ipc::SendData(ipc::FileRead::Response::MID,pkt.data,res);

		delete[] pkt.data;
		_packets.pop_back();
	}

	void write(ipc::IPCStream &is) {
		ipc::Client *c = get(is.fd());
		ipc::FileWrite::Request r;
		is >> r;

		Packet pkt;
		pkt.data = new uint8_t[r.count];
		pkt.size = r.count;

		if(r.shmemoff == -1)
			is >> ipc::ReceiveData(pkt.data,r.count);
		else
			memcpy(pkt.data,c->shm() + r.shmemoff,r.count);
		_packets.push_back(pkt);
		fcntl(id(),F_WAKE_READER,0);

		is << ipc::FileWrite::Response(r.count) << ipc::Send(ipc::FileWrite::Response::MID);
	}

	void getMac(ipc::IPCStream &is) {
		is << 0 << ipc::NIC::MAC() << ipc::Send(MSG_DEF_RESPONSE);
	}

private:
	std::list<Packet> _packets;
};

int main(void) {
	LoDevice dev("/dev/lo",0777);
	dev.loop();
	return EXIT_SUCCESS;
}
