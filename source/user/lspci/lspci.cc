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
#include <esc/cmdargs.h>
#include <ipc/proto/pci.h>
#include <stdio.h>
#include <stdlib.h>

#include "names.h"

static void printDevice(const ipc::PCI::Device *device,int verbose) {
	Vendor *vendor = PCINames::vendors.find(device->vendorId);
	Device *dev = NULL;
	if(vendor)
		dev = vendor->getDevice(device->deviceId);

	BaseClass *base = PCINames::classes.find(device->baseClass);
	SubClass *sub = NULL;
	ProgIF *pif = NULL;
	if(base) {
		sub = base->subclasses.find(device->subClass);
		if(sub)
			pif = sub->progifs.find(device->progInterface);
	}

	printf("%02x:%02x:%x ",device->bus,device->dev,device->func);

	if(base)
		printf("%s: ",base->getName().c_str());
	else
		printf("%02x: ",device->baseClass);
	if(sub)
		printf("%s (",sub->getName().c_str());
	else
		printf("%02x (",device->subClass);
	if(pif)
		printf("%s) ",pif->getName().c_str());
	else
		printf("%02x) ",device->progInterface);

	if(vendor)
		printf("%s ",vendor->getName().c_str());
	else
		printf("%04x ",device->vendorId);
	if(dev)
		printf("%s ",dev->getName().c_str());
	else
		printf("%04x ",device->vendorId);

	printf("(rev %02x)\n",device->revId);

	if(verbose) {
		printf("\tID: %04x:%04x\n",device->vendorId,device->deviceId);
		if(device->type == ipc::PCI::GENERIC) {
			size_t i;
			if(device->irq)
				printf("\tIRQ: %u\n",device->irq);
			for(i = 0; i < 6; i++) {
				if(device->bars[i].addr) {
					if(device->bars[i].type == ipc::PCI::Bar::BAR_MEM) {
						printf("\tMemory at %p (%d-bit, %s) [size=%zuK]\n",
								(void*)device->bars[i].addr,
								(device->bars[i].flags & ipc::PCI::Bar::BAR_MEM_32) ? 32 : 64,
								(device->bars[i].flags & ipc::PCI::Bar::BAR_MEM_PREFETCH)
									? "prefetchable" : "non-prefetchable",device->bars[i].size / 1024);
					}
					else {
						printf("\tI/O ports at %lx [size=%zu]\n",
								device->bars[i].addr,device->bars[i].size);
					}
				}
			}
		}
	}
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-v]\n",name);
	exit(EXIT_FAILURE);
}

// TODO temporary until we moved the collections into a general usable library
extern "C" void *cache_alloc(size_t size);
extern "C" void cache_free(void *ptr);

extern "C" void *cache_alloc(size_t size) {
	return malloc(size);
}
extern "C" void cache_free(void *ptr) {
	free(ptr);
}

int main(int argc,const char *argv[]) {
	int verbose = 0;
	int res = ca_parse(argc,argv,CA_NO_FREE,"v",&verbose);
	if(res < 0) {
		fprintf(stderr,"Invalid arguments: %s\n",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	PCINames::load(PCI_IDS_FILE);

	ipc::PCI pci("/dev/pci");
	size_t count = pci.getCount();
	for(size_t i = 0; i < count; ++i) {
		try {
			ipc::PCI::Device dev = pci.getByIndex(i);
			printDevice(&dev,verbose);
		}
		catch(const std::exception &e) {
			printe("%s",e.what());
		}
	}
	return EXIT_SUCCESS;
}
