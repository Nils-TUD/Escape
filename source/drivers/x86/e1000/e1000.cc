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
#include <esc/proto/pci.h>
#include <esc/proto/nic.h>
#include <esc/ipc/nicdevice.h>
#include <stdlib.h>

#include "e1000dev.h"

/* parts of the code are inspired by the iPXE intel driver */

struct IntelNIC {
	uint16_t vendor;
	uint16_t dev;
	const char *model;
	const char *name;
};

static IntelNIC nics[] = {
	{0x8086,0x0438,"dh8900cc","DH8900CC"},
	{0x8086,0x043a,"dh8900cc-f","DH8900CC Fiber"},
	{0x8086,0x043c,"dh8900cc-b","DH8900CC Backplane"},
	{0x8086,0x0440,"dh8900cc-s","DH8900CC SFP"},
	{0x8086,0x1000,"82542-f","82542 (Fiber)"},
	{0x8086,0x1001,"82543gc-f","82543GC (Fiber)"},
	{0x8086,0x1004,"82543gc","82543GC (Copper)"},
	{0x8086,0x1008,"82544ei","82544EI (Copper)"},
	{0x8086,0x1009,"82544ei-f","82544EI (Fiber)"},
	{0x8086,0x100c,"82544gc","82544GC (Copper)"},
	{0x8086,0x100d,"82544gc-l","82544GC (LOM)"},
	{0x8086,0x100e,"82540em","82540EM"},							/* tested and works */
	{0x8086,0x100f,"82545em","82545EM (Copper)"},
	{0x8086,0x1010,"82546eb","82546EB (Copper)"},
	{0x8086,0x1011,"82545em-f","82545EM (Fiber)"},
	{0x8086,0x1012,"82546eb-f","82546EB (Fiber)"},
	{0x8086,0x1013,"82541ei","82541EI"},
	{0x8086,0x1014,"82541er","82541ER"},
	{0x8086,0x1015,"82540em-l","82540EM (LOM)"},
	{0x8086,0x1016,"82540ep-m","82540EP (Mobile)"},
	{0x8086,0x1017,"82540ep","82540EP"},
	{0x8086,0x1018,"82541ei","82541EI"},
	{0x8086,0x1019,"82547ei","82547EI"},
	{0x8086,0x101a,"82547ei-m","82547EI (Mobile)"},
	{0x8086,0x101d,"82546eb","82546EB"},
	{0x8086,0x101e,"82540ep-m","82540EP (Mobile)"},
	{0x8086,0x1026,"82545gm","82545GM"},
	{0x8086,0x1027,"82545gm-1","82545GM"},
	{0x8086,0x1028,"82545gm-2","82545GM"},
	{0x8086,0x1049,"82566mm","82566MM"},
	{0x8086,0x104a,"82566dm","82566DM"},
	{0x8086,0x104b,"82566dc","82566DC"},
	{0x8086,0x104c,"82562v","82562V 10/100"},
	{0x8086,0x104d,"82566mc","82566MC"},
	{0x8086,0x105e,"82571eb","82571EB"},
	{0x8086,0x105f,"82571eb-1","82571EB"},
	{0x8086,0x1060,"82571eb-2","82571EB"},
	{0x8086,0x1075,"82547gi","82547GI"},
	{0x8086,0x1076,"82541gi","82541GI"},
	{0x8086,0x1077,"82541gi-1","82541GI"},
	{0x8086,0x1078,"82541er","82541ER"},
	{0x8086,0x1079,"82546gb","82546GB"},
	{0x8086,0x107a,"82546gb-1","82546GB"},
	{0x8086,0x107b,"82546gb-2","82546GB"},
	{0x8086,0x107c,"82541pi","82541PI"},
	{0x8086,0x107d,"82572ei","82572EI (Copper)"},
	{0x8086,0x107e,"82572ei-f","82572EI (Fiber)"},
	{0x8086,0x107f,"82572ei","82572EI"},
	{0x8086,0x108a,"82546gb-3","82546GB"},
	{0x8086,0x108b,"82573v","82573V (Copper)"},
	{0x8086,0x108c,"82573e","82573E (Copper)"},
	{0x8086,0x1096,"80003es2lan","80003ES2LAN (Copper)"},
	{0x8086,0x1098,"80003es2lan-s","80003ES2LAN (Serdes)"},
	{0x8086,0x1099,"82546gb-4","82546GB (Copper)"},
	{0x8086,0x109a,"82573l","82573L"},
	{0x8086,0x10a4,"82571eb","82571EB"},
	{0x8086,0x10a5,"82571eb","82571EB (Fiber)"},
	{0x8086,0x10a7,"82575eb","82575EB"},
	{0x8086,0x10a9,"82575eb","82575EB Backplane"},
	{0x8086,0x10b5,"82546gb","82546GB (Copper)"},
	{0x8086,0x10b9,"82572ei","82572EI (Copper)"},
	{0x8086,0x10ba,"80003es2lan","80003ES2LAN (Copper)"},
	{0x8086,0x10bb,"80003es2lan","80003ES2LAN (Serdes)"},
	{0x8086,0x10bc,"82571eb","82571EB (Copper)"},
	{0x8086,0x10bd,"82566dm-2","82566DM-2"},
	{0x8086,0x10bf,"82567lf","82567LF"},
	{0x8086,0x10c0,"82562v-2","82562V-2 10/100"},
	{0x8086,0x10c2,"82562g-2","82562G-2 10/100"},
	{0x8086,0x10c3,"82562gt-2","82562GT-2 10/100"},
	{0x8086,0x10c4,"82562gt","82562GT 10/100"},
	{0x8086,0x10c5,"82562g","82562G 10/100"},
	{0x8086,0x10c9,"82576","82576"},
	{0x8086,0x10cb,"82567v","82567V"},
	{0x8086,0x10cc,"82567lm-2","82567LM-2"},
	{0x8086,0x10cd,"82567lf-2","82567LF-2"},
	{0x8086,0x10ce,"82567v-2","82567V-2"},
	{0x8086,0x10d3,"82574l","82574L"},
	{0x8086,0x10d5,"82571pt","82571PT PT Quad"},
	{0x8086,0x10d6,"82575gb","82575GB"},
	{0x8086,0x10d9,"82571eb-d","82571EB Dual Mezzanine"},
	{0x8086,0x10da,"82571eb-q","82571EB Quad Mezzanine"},
	{0x8086,0x10de,"82567lm-3","82567LM-3"},
	{0x8086,0x10df,"82567lf-3","82567LF-3"},
	{0x8086,0x10e5,"82567lm-4","82567LM-4"},
	{0x8086,0x10e6,"82576","82576"},
	{0x8086,0x10e7,"82576-2","82576"},
	{0x8086,0x10e8,"82576-3","82576"},
	{0x8086,0x10ea,"82577lm","82577LM"},
	{0x8086,0x10eb,"82577lc","82577LC"},
	{0x8086,0x10ef,"82578dm","82578DM"},
	{0x8086,0x10f0,"82578dc","82578DC"},
	{0x8086,0x10f5,"82567lm","82567LM"},
	{0x8086,0x10f6,"82574l","82574L"},
	{0x8086,0x1501,"82567v-3","82567V-3"},
	{0x8086,0x1502,"82579lm","82579LM"},
	{0x8086,0x1503,"82579v","82579V"},
	{0x8086,0x150a,"82576ns","82576NS"},
	{0x8086,0x150c,"82583v","82583V"},
	{0x8086,0x150d,"82576-4","82576 Backplane"},
	{0x8086,0x150e,"82580","82580"},
	{0x8086,0x150f,"82580-f","82580 Fiber"},
	{0x8086,0x1510,"82580-b","82580 Backplane"},
	{0x8086,0x1511,"82580-s","82580 SFP"},
	{0x8086,0x1516,"82580-2","82580"},
	{0x8086,0x1518,"82576ns","82576NS SerDes"},
	{0x8086,0x1521,"i350","I350"},
	{0x8086,0x1522,"i350-f","I350 Fiber"},
	{0x8086,0x1523,"i350-b","I350 Backplane"},
	{0x8086,0x1524,"i350-2","I350"},
	{0x8086,0x1525,"82567v-4","82567V-4"},
	{0x8086,0x1526,"82576-5","82576"},
	{0x8086,0x1527,"82580-f2","82580 Fiber"},
	{0x8086,0x1533,"i210","I210"},
	{0x8086,0x153b,"i217","I217"},
	{0x8086,0x155a,"i218","I218"},									/* tested and works */
	{0x8086,0x294c,"82566dc-2","82566DC-2"},
	{0x8086,0x2e6e,"cemedia","CE Media Processor"},
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
			for(size_t i = 0; i < ARRAY_SIZE(nics); ++i) {
				if(nic.deviceId == nics[i].dev && nic.vendorId == nics[i].vendor) {
					found = true;
					print("Found PCI-device %d.%d.%d: vendor=%hx, device=%hx model=%s name=%s",
							nic.bus,nic.dev,nic.func,nic.vendorId,nic.deviceId,nics[i].model,nics[i].name);
					break;
				}
			}
		}
	}
	catch(...) {
		error("Unable to find an e1000");
	}

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
