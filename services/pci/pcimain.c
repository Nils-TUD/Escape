/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/service.h>
#include <esc/ports.h>
#include <esc/debug.h>
#include <stdlib.h>
#include <assert.h>
#include "pcilist.h"

#define IOPORT_PCI_CFG_DATA			0xCFC
#define IOPORT_PCI_CFG_ADDR			0xCF8

/* TODO according to http://en.wikipedia.org/wiki/PCI_configuration_space there are max. 256 buses.
 * Lost searches the first 8. why? */
#define BUS_COUNT					8
#define DEV_COUNT					32
#define FUNC_COUNT					8

#define VENDOR_INVALID				0xFFFF

/* header-types */
#define PCI_TYPE_STD				0
#define PCI_TYPE_PCIPCI_BRIDGE		1
#define PCI_TYPE_CARDBUS_BRIDGE		2

typedef struct {
	u8 bus;
	u8 dev;
	u8 func;
	u8 type;
	u16 deviceId;
	u16 vendorId;
	u8 baseClass;
	u8 subClass;
	u8 progInterface;
	u8 revId;
} sCPCIDevice;

static sPCIDevice *pci_getDevice(sCPCIDevice *dev);
static sPCIVendor *pci_getVendor(sCPCIDevice *dev);
static sPCIClassCode *pci_getClass(sCPCIDevice *dev);
static void pci_fillDev(sCPCIDevice *dev);
static u32 pci_read(u32 bus,u32 dev,u32 func,u32 offset);

int main(void) {
	sCPCIDevice device;
	u32 bus,dev,func;
	/* we want to access dwords... */
	if(requestIOPorts(IOPORT_PCI_CFG_DATA,4) < 0)
		error("Unable to request io-port %x\n",IOPORT_PCI_CFG_DATA);
	if(requestIOPorts(IOPORT_PCI_CFG_ADDR,4) < 0)
		error("Unable to request io-port %x\n",IOPORT_PCI_CFG_ADDR);

	tFile *f = fopen("/system/devices/pci","w");
	if(f == NULL)
		error("Unable to open /system/devices/pci");
	for(bus = 0; bus < BUS_COUNT; bus++) {
		for(dev = 0; dev < DEV_COUNT; dev++) {
			for(func = 0; func < FUNC_COUNT; func++) {
				device.bus = bus;
				device.dev = dev;
				device.func = func;
				pci_fillDev(&device);
				if(device.vendorId != VENDOR_INVALID) {
					sPCIVendor *pven = pci_getVendor(&device);
					sPCIDevice *pdev = pci_getDevice(&device);
					sPCIClassCode *pclass = pci_getClass(&device);
					fprintf(f,"%d:%d.%d:\n",bus,dev,func);
					fprintf(f,"	vendor = %04x (%s - %s)\n",device.vendorId,
							pven ? pven->vendorShort : "?",pven ? pven->vendorFull : "?");
					fprintf(f,"	device = %04x (%s - %s)\n",device.deviceId,
							pdev ? pdev->chip : "?",pdev ? pdev->chipDesc : "?");
					fprintf(f,"	rev = %x\n",device.revId);
					fprintf(f,"	class = %02x.%02x.%02x (%s - %s - %s)\n",device.baseClass,device.subClass,
							device.progInterface,pclass ? pclass->baseDesc : "?",
							pclass ? pclass->subDesc : "?",pclass ? pclass->progDesc : "?");
					fprintf(f,"	type=%x\n",device.type);
				}
			}
		}
	}
	fclose(f);

	releaseIOPorts(IOPORT_PCI_CFG_ADDR,4);
	releaseIOPorts(IOPORT_PCI_CFG_DATA,4);
	return EXIT_SUCCESS;
}

static sPCIDevice *pci_getDevice(sCPCIDevice *dev) {
	u32 i;
	for(i = 0; i < ARRAY_SIZE(pciDevices); i++) {
		if(pciDevices[i].vendorId == dev->vendorId && pciDevices[i].deviceId == dev->deviceId)
			return pciDevices + i;
	}
	return NULL;
}

static sPCIVendor *pci_getVendor(sCPCIDevice *dev) {
	u32 i;
	for(i = 0; i < ARRAY_SIZE(pciVendors); i++) {
		if(pciVendors[i].vendorId == dev->vendorId)
			return pciVendors + i;
	}
	return NULL;
}

static sPCIClassCode *pci_getClass(sCPCIDevice *dev) {
	u32 i;
	for(i = 0; i < ARRAY_SIZE(pciClassCodes); i++) {
		if(pciClassCodes[i].baseClass == dev->baseClass &&
				pciClassCodes[i].SubClass == dev->subClass &&
				pciClassCodes[i].progIf == dev->progInterface)
			return pciClassCodes + i;
	}
	return NULL;
}

static void pci_fillDev(sCPCIDevice *dev) {
	u32 val;
	val = pci_read(dev->bus,dev->dev,dev->func,0);
	dev->vendorId = val & 0xFFFF;
	dev->deviceId = val >> 16;
	/* it makes no sense to continue if the device is not present */
	if(dev->vendorId == VENDOR_INVALID)
		return;
	val = pci_read(dev->bus,dev->dev,dev->func,8);
	dev->revId = val & 0xFF;
	dev->baseClass = val >> 24;
	dev->subClass = (val >> 16) & 0xFF;
	dev->progInterface = (val >> 8) & 0xFF;
	val = pci_read(dev->bus,dev->dev,dev->func,0xC);
	dev->type = (val >> 16) & 0xFF;
}

static u32 pci_read(u32 bus,u32 dev,u32 func,u32 offset) {
	u32 addr = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC);
	outDWord(IOPORT_PCI_CFG_ADDR,addr);
	return inDWord(IOPORT_PCI_CFG_DATA);
}
