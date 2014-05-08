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
#include <ipc/proto/pci.h>
#include <ipc/proto/nic.h>
#include <ipc/nicdevice.h>
#include <stdlib.h>

#include "e1000dev.h"

static struct {
	uint16_t dev;
	uint16_t vendor;
} supported[] = {
	{0x100e,0x8086},
};

int main(int argc,char **argv) {
	if(argc != 2)
		error("Usage: %s <device>\n",argv[0]);

	ipc::PCI pci("/dev/pci");

	// find e1000 NIC
	ipc::PCI::Device nic;
	try {
		bool found = false;
		for(int no = 0; !found; ++no) {
			nic = pci.getByClass(ipc::NIC::PCI_CLASS,ipc::NIC::PCI_SUBCLASS,no);
			for(size_t i = 0; i < ARRAY_SIZE(supported); ++i) {
				if(nic.deviceId == supported[i].dev && nic.vendorId == supported[i].vendor) {
					found = true;
					break;
				}
			}
		}
	}
	catch(...) {
		error("Unable to find an e1000");
	}

	print("Found PCI-device %d.%d.%d: vendor=%hx, device=%hx",
			nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId);

	E1000 *e1000 = new E1000(pci,nic);
	ipc::NICDevice dev(argv[1],0770,e1000);
	e1000->setHandler(std::make_memfun(&dev,&ipc::NICDevice::checkPending));

	ipc::NIC::MAC mac = dev.mac();
	print("NIC has MAC address %02x:%02x:%02x:%02x:%02x:%02x",
		mac.bytes()[0],mac.bytes()[1],mac.bytes()[2],mac.bytes()[3],mac.bytes()[4],mac.bytes()[5]);
	fflush(stdout);

	dev.loop();
	return 0;
}
