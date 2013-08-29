/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/driver/pci.h>
#include <stdio.h>
#include <stdlib.h>

#include "pcilist.h"

static void printDevice(sPCIDevice *device,int verbose);
static sPCIDeviceInfo *getDevice(sPCIDevice *dev);
static sPCIVendor *getVendor(sPCIDevice *dev);
static sPCIClassCode *getClass(sPCIDevice *dev);

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-v]\n",name);
	exit(EXIT_FAILURE);
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

	ssize_t i, count = pci_getCount();
	if(count < 0)
		error("Unable to count PCI devices");
	for(i = 0; i < count; ++i) {
		sPCIDevice dev;
		if(pci_getByIdx(i,&dev) < 0) {
			printe("Unable to get information about device %zu",i);
			continue;
		}
		printDevice(&dev,verbose);
	}
	return EXIT_SUCCESS;
}

static void printDevice(sPCIDevice *device,int verbose) {
	sPCIVendor *pven = getVendor(device);
	sPCIDeviceInfo *pdev = getDevice(device);
	sPCIClassCode *pclass = getClass(device);
	printf("%02x:%02x:%x %s: %s %s (rev %02x)\n",
	       device->bus,device->dev,device->func,
	       pclass ? pclass->baseDesc : "?",
	       pven ? pven->vendorFull : "?",
	       pdev ? pdev->chipDesc : "?",
		   device->revId);
	if(verbose) {
		printf("\tID: %04x:%04x\n",device->vendorId,device->deviceId);
		if(device->type == PCI_TYPE_GENERIC) {
			size_t i;
			if(device->irq)
				printf("\tIRQ: %u\n",device->irq);
			for(i = 0; i < 6; i++) {
				if(device->bars[i].addr) {
					if(device->bars[i].type == BAR_MEM) {
						printf("\tMemory at %p (%d-bit, %s) [size=%zuK]\n",
								device->bars[i].addr,device->bars[i].flags & BAR_MEM_32 ? 32 : 64,
								(device->bars[i].flags & BAR_MEM_PREFETCH)
									? "prefetchable" : "non-prefetchable",device->bars[i].size / 1024);
					}
					else {
						printf("\tI/O ports at %x [size=%zu]\n",
								device->bars[i].addr,device->bars[i].size);
					}
				}
			}
		}
	}
}

static sPCIDeviceInfo *getDevice(sPCIDevice *dev) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(pciDevices); i++) {
		if(pciDevices[i].vendorId == dev->vendorId && pciDevices[i].deviceId == dev->deviceId)
			return pciDevices + i;
	}
	return NULL;
}

static sPCIVendor *getVendor(sPCIDevice *dev) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(pciVendors); i++) {
		if(pciVendors[i].vendorId == dev->vendorId)
			return pciVendors + i;
	}
	return NULL;
}

static sPCIClassCode *getClass(sPCIDevice *dev) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(pciClassCodes); i++) {
		if(pciClassCodes[i].baseClass == dev->baseClass &&
				pciClassCodes[i].SubClass == dev->subClass &&
				pciClassCodes[i].progIf == dev->progInterface)
			return pciClassCodes + i;
	}
	return NULL;
}
