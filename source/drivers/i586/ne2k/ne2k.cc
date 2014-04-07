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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ne2kdev.h"

class Ne2kDevice : public ipc::ClientDevice<> {
public:
	explicit Ne2kDevice(const char *path,mode_t mode,const ipc::PCI::Device &nic)
		: ipc::ClientDevice<>(path,mode,DEV_TYPE_CHAR,DEV_SHFILE | DEV_READ | DEV_WRITE),
		  _ne2k(nic,id()) {
		set(MSG_FILE_READ,std::make_memfun(this,&Ne2kDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&Ne2kDevice::write));
		set(MSG_NIC_GETMAC,std::make_memfun(this,&Ne2kDevice::getMac));
	}

	void read(ipc::IPCStream &is) {
		ipc::Client *c = get(is.fd());
		ipc::FileRead::Request r;
		is >> r;

		char *data = NULL;
		if(r.shmemoff != -1)
			data = c->shm() + r.shmemoff;
		else
			data = new char[r.count];
		ssize_t res = _ne2k.fetch(data,r.count);

		is << ipc::FileRead::Response(res) << ipc::Send(ipc::FileRead::Response::MID);
		if(r.shmemoff == -1 && res) {
			is << ipc::SendData(ipc::FileRead::Response::MID,data,res);
			delete[] data;
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
		is << ipc::FileWrite::Response(res) << ipc::Send(ipc::FileWrite::Response::MID);
		if(r.shmemoff == -1)
			delete[] data;
	}

	void getMac(ipc::IPCStream &is) {
		is << 0 << _ne2k.mac() << ipc::Send(MSG_DEF_RESPONSE);
	}

private:
	Ne2k _ne2k;
};

int main(void) {
	ipc::PCI pci("/dev/pci");
	ipc::PCI::Device nic = pci.getByClass(Ne2k::NIC_CLASS,Ne2k::NIC_SUBCLASS);
	if(nic.deviceId != Ne2k::DEVICE_ID || nic.vendorId != Ne2k::VENDOR_ID) {
		error("NIC is no NE2K (found %d.%d.%d: vendor=%hx, device=%hx)",
				nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId);
	}

	printf("[ne2k] found PCI-device %d.%d.%d: vendor=%hx, device=%hx\n",
			nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId);
	fflush(stdout);

	Ne2kDevice dev("/dev/ne2k",0770,nic);
	dev.loop();
	return EXIT_SUCCESS;
}
