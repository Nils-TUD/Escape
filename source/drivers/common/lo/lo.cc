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

#include <sys/common.h>
#include <esc/proto/nic.h>
#include <esc/ipc/nicdevice.h>
#include <stdlib.h>
#include <stdio.h>

class LoDriver : public ipc::NICDriver {
public:
	explicit LoDriver() : ipc::NICDriver(), handler() {
	}

	virtual ipc::NIC::MAC mac() const {
		return ipc::NIC::MAC();
	}
	virtual ulong mtu() const {
		return 64 * 1024;
	}
	virtual ssize_t send(const void *packet,size_t size) {
		Packet *pkt = (Packet*)malloc(sizeof(Packet) + size);
		pkt->length = size;
		memcpy(pkt->data,packet,size);
		insert(pkt);
		(*handler)();
		return size;
	}

	std::Functor<void> *handler;
};

int main(int argc,char **argv) {
	if(argc != 2)
		error("Usage: %s <device>\n",argv[0]);

	LoDriver *lo = new LoDriver();
	ipc::NICDevice dev(argv[1],0777,lo);
	lo->handler = std::make_memfun(&dev,&ipc::NICDevice::checkPending);
	dev.loop();
	return EXIT_SUCCESS;
}
