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
#include <esc/arch/i586/ports.h>
#include <esc/sllist.h>
#include <esc/dir.h>
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

#define BAR_OFFSET					0x10

#define BAR_ADDR_IO_MASK			0xFFFFFFFC
#define BAR_ADDR_MEM_MASK			0xFFFFFFF0

#define BAR_ADDR_SPACE				0x01
#define BAR_ADDR_SPACE_IO			0x01
#define BAR_ADDR_SPACE_MEM			0x00

static void list_detect(void);
static void list_createVFSEntry(sPCIDevice *device);
static sPCIDeviceInfo *pci_getDevice(sPCIDevice *dev);
static sPCIVendor *pci_getVendor(sPCIDevice *dev);
static sPCIClassCode *pci_getClass(sPCIDevice *dev);
static void pci_fillDev(sPCIDevice *dev);
static void pci_fillBar(sPCIDevice *dev,size_t i);
static uint32_t pci_read(uchar bus,uchar dev,uchar func,uchar offset);
static void pci_write(uchar bus,uchar dev,uchar func,uchar offset,uint32_t value);

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

sPCIDevice *list_getByClass(uchar baseClass,uchar subClass) {
	sSLNode *n;
	for(n = sll_begin(devices); n != NULL; n = n->next) {
		sPCIDevice *d = (sPCIDevice*)n->data;
		if(d->baseClass == baseClass && d->subClass == subClass)
			return d;
	}
	return NULL;
}

sPCIDevice *list_getById(uchar bus,uchar dev,uchar func) {
	sSLNode *n;
	for(n = sll_begin(devices); n != NULL; n = n->next) {
		sPCIDevice *d = (sPCIDevice*)n->data;
		if(d->bus == bus && d->dev == dev && d->func == func)
			return d;
	}
	return NULL;
}

static void list_detect(void) {
	uchar bus,dev,func;
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
		size_t i;
		fprintf(f,"irq: 		%u\n",device->irq);
		fprintf(f,"bars:\n");
		for(i = 0; i < 6; i++) {
			if(device->bars[i].addr) {
				fprintf(f,"  type=%d, addr=%#08x, size=%u\n",
						device->bars[i].type,device->bars[i].addr,device->bars[i].size);
			}
		}
	}
	fclose(f);
}

static sPCIDeviceInfo *pci_getDevice(sPCIDevice *dev) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(pciDevices); i++) {
		if(pciDevices[i].vendorId == dev->vendorId && pciDevices[i].deviceId == dev->deviceId)
			return pciDevices + i;
	}
	return NULL;
}

static sPCIVendor *pci_getVendor(sPCIDevice *dev) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(pciVendors); i++) {
		if(pciVendors[i].vendorId == dev->vendorId)
			return pciVendors + i;
	}
	return NULL;
}

static sPCIClassCode *pci_getClass(sPCIDevice *dev) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(pciClassCodes); i++) {
		if(pciClassCodes[i].baseClass == dev->baseClass &&
				pciClassCodes[i].SubClass == dev->subClass &&
				pciClassCodes[i].progIf == dev->progInterface)
			return pciClassCodes + i;
	}
	return NULL;
}

static void pci_fillDev(sPCIDevice *dev) {
	uint32_t val;
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
	dev->irq = 0;
	if(dev->type == 0x00) {
		size_t i;
		for(i = 0; i < 6; i++)
			pci_fillBar(dev,i);
		dev->irq = pci_read(dev->bus,dev->dev,dev->func,0x3C) & 0xFF;
	}
}

static void pci_fillBar(sPCIDevice *dev,size_t i) {
	sPCIBar *bar = dev->bars + i;
	uint32_t barValue = pci_read(dev->bus,dev->dev,dev->func,BAR_OFFSET + i * 4);
	bar->type = barValue & 0x1;
	bar->addr = barValue & ~0xF;

	/* to get the size, we set all bits and read it back to see what bits are still set */
	pci_write(dev->bus,dev->dev,dev->func,BAR_OFFSET + i * 4,0xFFFFFFF0 | bar->type);
	bar->size = pci_read(dev->bus,dev->dev,dev->func,BAR_OFFSET + i * 4);
	if(bar->size == 0 || bar->size == 0xFFFFFFFF)
		bar->size = 0;
	else {
		if((bar->size & BAR_ADDR_SPACE) == BAR_ADDR_SPACE_MEM)
			bar->size &= BAR_ADDR_MEM_MASK;
		else
			bar->size &= BAR_ADDR_IO_MASK;
		bar->size &= ~(bar->size - 1);
	}
	pci_write(dev->bus,dev->dev,dev->func,BAR_OFFSET + i * 4,barValue);
}

static uint32_t pci_read(uchar bus,uchar dev,uchar func,uchar offset) {
	uint32_t addr = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC);
	outDWord(IOPORT_PCI_CFG_ADDR,addr);
	return inDWord(IOPORT_PCI_CFG_DATA);
}

static void pci_write(uchar bus,uchar dev,uchar func,uchar offset,uint32_t value) {
	uint32_t addr = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC);
	outDWord(IOPORT_PCI_CFG_ADDR,addr);
	outDWord(IOPORT_PCI_CFG_DATA,value);
}
