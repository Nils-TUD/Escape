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
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <stdlib.h>

#include "names.h"

using namespace esc;

static const char *caps[] = {
	/* 0x00 */ "??",
	/* 0x01 */ "Power Management",
	/* 0x02 */ "Accelerated Graphics Port",
	/* 0x03 */ "Vital Product Data",
	/* 0x04 */ "Slot Identification",
	/* 0x05 */ "Message Signalled Interrupts",
	/* 0x06 */ "CompactPCI HotSwap",
	/* 0x07 */ "PCI-X",
	/* 0x08 */ "HyperTransport",
	/* 0x09 */ "Vendor specific",
	/* 0x0A */ "Debug port",
	/* 0x0B */ "CompactPCI Central Resource Control",
	/* 0x0C */ "PCI Standard Hot-Plug Controller",
	/* 0x0D */ "Bridge subsystem vendor/device ID",
	/* 0x0E */ "AGP Target PCI-PCI bridge",
	/* 0x0F */ "Secure Device",
	/* 0x10 */ "PCI Express",
	/* 0x11 */ "MSI-X",
	/* 0x12 */ "SATA Data/Index Conf.",
	/* 0x13 */ "PCI Advanced Features",
};

static void printDevice(PCI &pci,const PCI::Device *d,int verbose) {
	Vendor *vendor = PCINames::vendors.find(d->vendorId);
	Device *dev = NULL;
	if(vendor)
		dev = vendor->getDevice(d->deviceId);

	BaseClass *base = PCINames::classes.find(d->baseClass);
	SubClass *sub = NULL;
	ProgIF *pif = NULL;
	if(base) {
		sub = base->subclasses.find(d->subClass);
		if(sub)
			pif = sub->progifs.find(d->progInterface);
	}

	sout << fmt(d->bus,"0x",2) << ":" << fmt(d->dev,"0x",2) << ":" << fmt(d->func,"x") << " ";

	if(base)
		sout << base->getName() << ": ";
	else
		sout << fmt(d->baseClass,"0x",2) << ": ";
	if(sub)
		sout << sub->getName() << " (";
	else
		sout << fmt(d->subClass,"0x",2) << " (";
	if(pif)
		sout << pif->getName() << ") ";
	else
		sout << fmt(d->progInterface,"0x",2) << ") ";
	if(vendor)
		sout << vendor->getName() << " ";
	else
		sout << fmt(d->vendorId,"0x",4) << " ";
	if(dev)
		sout << dev->getName() << " ";
	else
		sout << fmt(d->deviceId,"0x",4) << " ";

	sout << "(rev " << fmt(d->revId,"0x",2) << ")\n";

	if(verbose) {
		sout << "        ID: " << fmt(d->vendorId,"0x",4) << ":" << fmt(d->deviceId,"0x",4) << "\n";
		if(d->type == PCI::GENERIC) {
			size_t i;
			if(d->irq)
				sout << "        IRQ: " << d->irq << "\n";
			for(i = 0; i < 6; i++) {
				if(d->bars[i].addr) {
					if(d->bars[i].type == PCI::Bar::BAR_MEM) {
						const char *pref = (d->bars[i].flags & PCI::Bar::BAR_MEM_PREFETCH)
											? "prefetchable" : "non-prefetchable";
						sout << "        Memory at " << ((void*)d->bars[i].addr) << " ";
						sout << "(" << ((d->bars[i].flags & PCI::Bar::BAR_MEM_32) ? 32 : 64) << "-bit";
						sout << ", " << pref << ") [size=" << (d->bars[i].size / 1024) << "K]\n";
					}
					else {
						sout << "        I/O ports at " << fmt(d->bars[i].addr,"x");
						sout << " [size=" << d->bars[i].size << "]\n";
					}
				}
			}

			uint32_t val = pci.read(d->bus,d->dev,d->func,0x4);
			uint16_t status = val >> 16;
			sout << "        Status: ";
			if(status & PCI::ST_IRQ)
				sout << "IRQ ";
			if(status & PCI::ST_CAPS)
				sout << "CAPS ";
			sout << "\n";

			if(status & PCI::ST_CAPS) {
	            uint8_t offset = pci.read(d->bus,d->dev,d->func,0x34);
            	while((offset != 0) && !(offset & 0x3)) {
            		uint32_t capidx = pci.read(d->bus,d->dev,d->func,offset);
            		const char *name = (capidx & 0xFF) < ARRAY_SIZE(caps) ? caps[capidx & 0xFF] : "??";
            		sout << "        Capabilities: [" << fmt(offset,"x",3) << "] " << name << "\n";
            		offset = capidx >> 8;
            	}
	        }
		}
	}
}

static void usage(const char *name) {
	serr << "Usage: " << name << " [-v]\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	int verbose = 0;
	cmdargs args(argc,argv,cmdargs::NO_FREE);
	try {
		args.parse("v",&verbose);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	PCINames::load(PCI_IDS_FILE);

	PCI pci("/dev/pci");
	size_t count = pci.getCount();
	for(size_t i = 0; i < count; ++i) {
		try {
			PCI::Device dev = pci.getByIndex(i);
			printDevice(pci,&dev,verbose);
		}
		catch(const std::exception &e) {
			errmsg(e.what());
		}
	}
	return EXIT_SUCCESS;
}
