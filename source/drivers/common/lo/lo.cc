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

#include <esc/ipc/nicdevice.h>
#include <esc/proto/nic.h>
#include <sys/common.h>
#include <stdio.h>
#include <stdlib.h>

class LoDriver : public esc::NICDriver {
public:
	explicit LoDriver() : esc::NICDriver(), handler() {
	}

	virtual esc::NIC::MAC mac() const {
		return esc::NIC::MAC();
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
	if(argc != 3)
		error("Usage: %s <bdf> <device>\n",argv[0]);

	LoDriver *lo = new LoDriver();
	esc::NICDevice dev(argv[2],0700,lo);
	lo->handler = std::make_memfun(&dev,&esc::NICDevice::checkPending);
	dev.loop();
	return EXIT_SUCCESS;
}
