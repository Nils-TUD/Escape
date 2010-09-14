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
#include <esc/sllist.h>
#include <esc/dir.h>
#include <esc/ports.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "pcilist.h"

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

static void list_detect(void);
static void list_createVFSEntry(sPCIDevice *device);
static sPCIDeviceInfo *pci_getDevice(sPCIDevice *dev);
static sPCIVendor *pci_getVendor(sPCIDevice *dev);
static sPCIClassCode *pci_getClass(sPCIDevice *dev);
static void pci_fillDev(sPCIDevice *dev);
static u32 pci_read(u32 bus,u32 dev,u32 func,u32 offset);

static sSLList *devices;

void list_init(void) {
	devices = sll_create();
	if(!devices)
		error("Unable to create device-list");

	/* we want to access dwords... */
	if(requestIOPorts(IOPORT_PCI_CFG_DATA,4) < 0)
		error("Unable to request io-port %x",IOPORT_PCI_CFG_DATA);
	if(requestIOPorts(IOPORT_PCI_CFG_ADDR,4) < 0)
		error("Unable to request io-port %x",IOPORT_PCI_CFG_ADDR);

	if(mkdir("/system/devices/pci") < 0)
		error("Unable to create pci-directory");
	list_detect();
}

sPCIDevice *list_getByClass(u8 baseClass,u8 subClass) {
	sSLNode *n;
	for(n = sll_begin(devices); n != NULL; n = n->next) {
		sPCIDevice *d = (sPCIDevice*)n->data;
		if(d->baseClass == baseClass && d->subClass == subClass)
			return d;
	}
	return NULL;
}

sPCIDevice *list_getById(u8 bus,u8 dev,u8 func) {
	sSLNode *n;
	for(n = sll_begin(devices); n != NULL; n = n->next) {
		sPCIDevice *d = (sPCIDevice*)n->data;
		if(d->bus == bus && d->dev == dev && d->func == func)
			return d;
	}
	return NULL;
}

static void list_detect(void) {
	u8 bus,dev,func;
	for(bus = 0; bus < BUS_COUNT; bus++) {
		for(dev = 0; dev < DEV_COUNT; dev++) {
			for(func = 0; func < FUNC_COUNT; func++) {
				sPCIDevice *device = (sPCIDevice*)malloc(sizeof(sPCIDevice));
				if(!device)
					error("Unable to create device");
				device->bus = bus;
				device->dev = dev;
				device->func = func;
				pci_fillDev(device);
				if(device->vendorId != VENDOR_INVALID) {
					if(!sll_append(devices,device))
						error("Unable to append device");
					list_createVFSEntry(device);
				}
				else
					free(device);
			}
		}
	}
}

static void list_createVFSEntry(sPCIDevice *device) {
	sPCIVendor *pven;
	sPCIDeviceInfo *pdev;
	sPCIClassCode *pclass;
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/system/devices/pci/%u.%u.%u",
			device->bus,device->dev,device->func);
	FILE *f = fopen(path,"w");
	if(f == NULL)
		error("Unable to open '%s'",path);
	pven = pci_getVendor(device);
	pdev = pci_getDevice(device);
	pclass = pci_getClass(device);
	fprintf(f,"vendor:		%04x (%s - %s)\n",device->vendorId,
			pven ? pven->vendorShort : "?",pven ? pven->vendorFull : "?");
	fprintf(f,"device:		%04x (%s - %s)\n",device->deviceId,
			pdev ? pdev->chip : "?",pdev ? pdev->chipDesc : "?");
	fprintf(f,"rev:		%x\n",device->revId);
	fprintf(f,"class:		%02x.%02x.%02x (%s - %s - %s)\n",device->baseClass,device->subClass,
			device->progInterface,pclass ? pclass->baseDesc : "?",
			pclass ? pclass->subDesc : "?",pclass ? pclass->progDesc : "?");
	fprintf(f,"type: 		%x\n",device->type);
	if(device->type == 0x00) {
		u32 i;
		fprintf(f,"bars: 		");
		for(i = 0; i < 6; i++)
			fprintf(f,"%08x ",device->bars[i]);
		fprintf(f,"\n");
	}
	fclose(f);
}

static sPCIDeviceInfo *pci_getDevice(sPCIDevice *dev) {
	u32 i;
	for(i = 0; i < ARRAY_SIZE(pciDevices); i++) {
		if(pciDevices[i].vendorId == dev->vendorId && pciDevices[i].deviceId == dev->deviceId)
			return pciDevices + i;
	}
	return NULL;
}

static sPCIVendor *pci_getVendor(sPCIDevice *dev) {
	u32 i;
	for(i = 0; i < ARRAY_SIZE(pciVendors); i++) {
		if(pciVendors[i].vendorId == dev->vendorId)
			return pciVendors + i;
	}
	return NULL;
}

static sPCIClassCode *pci_getClass(sPCIDevice *dev) {
	u32 i;
	for(i = 0; i < ARRAY_SIZE(pciClassCodes); i++) {
		if(pciClassCodes[i].baseClass == dev->baseClass &&
				pciClassCodes[i].SubClass == dev->subClass &&
				pciClassCodes[i].progIf == dev->progInterface)
			return pciClassCodes + i;
	}
	return NULL;
}

static void pci_fillDev(sPCIDevice *dev) {
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
	if(dev->type == 0x00) {
		u32 i;
		for(i = 0; i < 6; i++)
			dev->bars[i] = pci_read(dev->bus,dev->dev,dev->func,0x10 + i * 4);
	}
}

static u32 pci_read(u32 bus,u32 dev,u32 func,u32 offset) {
	u32 addr = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC);
	outDWord(IOPORT_PCI_CFG_ADDR,addr);
	return inDWord(IOPORT_PCI_CFG_DATA);
}
