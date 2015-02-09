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

#include <esc/ipc/clientdevice.h>
#include <esc/ipc/nicdevice.h>
#include <esc/proto/nic.h>
#include <esc/proto/pci.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <assert.h>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>

#include "ne2kdev.h"

int main(int argc,char **argv) {
	if(argc != 2)
		error("Usage: %s <device>\n",argv[0]);

	esc::PCI pci("/dev/pci");

	// find Ne2000 NIC
	esc::PCI::Device nic;
	try {
		for(int no = 0; ; ++no) {
			nic = pci.getByClass(esc::NIC::PCI_CLASS,esc::NIC::PCI_SUBCLASS,no);
			if(nic.deviceId == Ne2k::DEVICE_ID && nic.vendorId == Ne2k::VENDOR_ID)
				break;
		}
	}
	catch(...) {
		error("Unable to find an NE2K (vendor=%hx, device=%hx)",Ne2k::DEVICE_ID,Ne2k::VENDOR_ID);
	}

	print("Found PCI-device %d.%d.%d: vendor=%hx, device=%hx",
			nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId);

	Ne2k *ne2k = new Ne2k(pci,nic);
	esc::NICDevice dev(argv[1],0770,ne2k);
	ne2k->start(std::make_memfun(&dev,&esc::NICDevice::checkPending));

	esc::NIC::MAC mac = dev.mac();
	print("NIC has MAC address %02x:%02x:%02x:%02x:%02x:%02x",
		mac.bytes()[0],mac.bytes()[1],mac.bytes()[2],mac.bytes()[3],mac.bytes()[4],mac.bytes()[5]);
	fflush(stdout);

	dev.loop();
	return EXIT_SUCCESS;
}
