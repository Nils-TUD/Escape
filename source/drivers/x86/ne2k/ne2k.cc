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

#include <esc/proto/pci.h>
#include <esc/stream/istringstream.h>
#include <sys/common.h>
#include <stdlib.h>

#include "ne2kdev.h"

int main(int argc,char **argv) {
	if(argc != 3)
		error("Usage: %s <bdf> <path>\n",argv[0]);

	Ne2k *ne2k;
	{
		esc::PCI pci("/dev/pci");

		uchar bus,dev,func;
		esc::IStringStream is(argv[1]);
		is >> bus; is.get(); is >> dev; is.get(); is >> func;

		esc::PCI::Device nic = pci.getById(bus,dev,func);

		print("Using PCI-device %d.%d.%d: vendor=%hx, device=%hx",
				nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId);

		ne2k = new Ne2k(pci,nic);
	}

	esc::NICDevice nicdev(argv[2],0770,ne2k);
	ne2k->start(std::make_memfun(&nicdev,&esc::NICDevice::checkPending));

	esc::NIC::MAC mac = nicdev.mac();
	print("NIC has MAC address %02x:%02x:%02x:%02x:%02x:%02x",
		mac.bytes()[0],mac.bytes()[1],mac.bytes()[2],mac.bytes()[3],mac.bytes()[4],mac.bytes()[5]);
	fflush(stdout);

	nicdev.loop();
	return EXIT_SUCCESS;
}
