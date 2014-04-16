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
#include <esc/messages.h>
#include <esc/driver.h>
#include <esc/mem.h>
#include <esc/io.h>
#include <ipc/proto/nic.h>
#include <ipc/proto/pci.h>
#include <ipc/clientdevice.h>
#include <ipc/requestqueue.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <mutex>

#include "ne2kdev.h"

class Ne2kDevice : public ipc::ClientDevice<> {
public:
	explicit Ne2kDevice(const char *path,mode_t mode,const ipc::PCI::Device &nic)
		: ipc::ClientDevice<>(path,mode,DEV_TYPE_CHAR,DEV_CANCEL | DEV_SHFILE | DEV_READ | DEV_WRITE),
		  _ne2k(nic,id(),std::make_memfun(this,&Ne2kDevice::checkPending)),
		  _requests(std::make_memfun(this,&Ne2kDevice::handleRead)), _mutex() {
		set(MSG_DEV_CANCEL,std::make_memfun(this,&Ne2kDevice::cancel));
		set(MSG_FILE_READ,std::make_memfun(this,&Ne2kDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&Ne2kDevice::write));
		set(MSG_NIC_GETMAC,std::make_memfun(this,&Ne2kDevice::getMac));
	}

	ipc::NIC::MAC mac() const {
		return _ne2k.mac();
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
		else {
			std::lock_guard<std::mutex> guard(_mutex);
			res = _requests.cancel(mid);
		}

		is << res << ipc::Reply();
	}

	void read(ipc::IPCStream &is) {
		ipc::Client *c = get(is.fd());
		ipc::FileRead::Request r;
		is >> r;

		char *data = NULL;
		if(r.shmemoff != -1)
			data = c->shm() + r.shmemoff;

		if(!handleRead(is.fd(),is.msgid(),data,r.count)) {
			std::lock_guard<std::mutex> guard(_mutex);
			_requests.enqueue(ipc::Request(is.fd(),is.msgid(),data,r.count));
		}
	}

	void write(ipc::IPCStream &is) {
		char *data;
		ipc::FileWrite::Request r;
		is >> r;
		if(r.shmemoff == -1) {
			data = new char[r.count];
			is >> ipc::ReceiveData(data,r.count);
		}
		else
			data = get(is.fd())->shm() + r.shmemoff;

		ssize_t res = _ne2k.send(data,r.count);
		is << ipc::FileWrite::Response(res) << ipc::Reply();
		if(r.shmemoff == -1)
			delete[] data;
	}

	void getMac(ipc::IPCStream &is) {
		is << 0 << _ne2k.mac() << ipc::Reply();
	}

private:
	// called from ne2k's receive routine
	void checkPending() {
		std::lock_guard<std::mutex> guard(_mutex);
		_requests.handle();
	}

	bool handleRead(int fd,msgid_t mid,char *data,size_t count) {
		Ne2k::Packet *pkt = _ne2k.fetch();
		if(!pkt)
			return false;

		ssize_t res = count >= pkt->length ? pkt->length : -ENOMEM;
		if(data && res > 0)
			memcpy(data,pkt->data,res);

		ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
		ipc::IPCStream is(fd,buffer,sizeof(buffer),mid);
		is << ipc::FileRead::Response(res) << ipc::Reply();
		if(!data && res > 0)
			is << ipc::ReplyData(pkt->data,res);

		free(pkt);
		return true;
	}

	Ne2k _ne2k;
	ipc::RequestQueue _requests;
	std::mutex _mutex;
};

int main(int argc,char **argv) {
	if(argc != 2)
		error("Usage: %s <device>\n",argv[0]);

	ipc::PCI pci("/dev/pci");
	ipc::PCI::Device nic = pci.getByClass(ipc::NIC::PCI_CLASS,ipc::NIC::PCI_SUBCLASS);
	if(nic.deviceId != Ne2k::DEVICE_ID || nic.vendorId != Ne2k::VENDOR_ID) {
		error("NIC is no NE2K (found %d.%d.%d: vendor=%hx, device=%hx)",
				nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId);
	}

	print("Found PCI-device %d.%d.%d: vendor=%hx, device=%hx",
			nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId);

	Ne2kDevice dev(argv[1],0770,nic);

	ipc::NIC::MAC mac = dev.mac();
	print("NIC has MAC address %02x:%02x:%02x:%02x:%02x:%02x",
		mac.bytes()[0],mac.bytes()[1],mac.bytes()[2],mac.bytes()[3],mac.bytes()[4],mac.bytes()[5]);
	fflush(stdout);

	dev.loop();
	return EXIT_SUCCESS;
}
