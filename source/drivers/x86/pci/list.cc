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
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "list.h"
#include "pci.h"

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
#define BAR_MEM_TYPE(bar)			(((bar) >> 1) & 0x3)
#define BAR_MEM_PREFETCHABLE(bar)	(((bar) >> 3) & 0x1)

static void list_detect(void);
static void list_fillDev(esc::PCI::Device *dev);
static void list_fillBar(esc::PCI::Device *dev,size_t i);

static std::vector<esc::PCI::Device*> devices;

void list_init(void) {
	/* we want to access dwords... */
	if(reqports(IOPORT_PCI_CFG_DATA,4) < 0)
		error("Unable to request io-port %x",IOPORT_PCI_CFG_DATA);
	if(reqports(IOPORT_PCI_CFG_ADDR,4) < 0)
		error("Unable to request io-port %x",IOPORT_PCI_CFG_ADDR);

	list_detect();
}

esc::PCI::Device *list_getByClass(uchar baseClass,uchar subClass,int no) {
	for(auto d = devices.begin(); d != devices.end(); ++d) {
		if((*d)->baseClass == baseClass && (*d)->subClass == subClass) {
			if(no-- == 0)
				return *d;
		}
	}
	return NULL;
}

esc::PCI::Device *list_getById(uchar bus,uchar dev,uchar func) {
	for(auto d = devices.begin(); d != devices.end(); ++d) {
		if((*d)->bus == bus && (*d)->dev == dev && (*d)->func == func)
			return *d;
	}
	return NULL;
}

esc::PCI::Device *list_get(size_t i) {
	if(i >= devices.size())
		return NULL;
	return devices[i];
}

size_t list_length(void) {
	return devices.size();
}

static void list_detect(void) {
	uchar bus,dev,func;
	for(bus = 0; bus < BUS_COUNT; bus++) {
		for(dev = 0; dev < DEV_COUNT; dev++) {
			for(func = 0; func < FUNC_COUNT; func++) {
				esc::PCI::Device *device = (esc::PCI::Device*)malloc(sizeof(esc::PCI::Device));
				if(!device)
					error("Unable to create device");
				device->bus = bus;
				device->dev = dev;
				device->func = func;
				list_fillDev(device);
				if(device->vendorId != VENDOR_INVALID)
					devices.push_back(device);
				else
					free(device);
			}
		}
	}
}

static void list_fillDev(esc::PCI::Device *dev) {
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
	if(dev->type == esc::PCI::GENERIC) {
		size_t i;
		for(i = 0; i < 6; i++)
			list_fillBar(dev,i);
		dev->irq = pci_read(dev->bus,dev->dev,dev->func,0x3C) & 0xFF;
	}
}

static void list_fillBar(esc::PCI::Device *dev,size_t i) {
	esc::PCI::Bar *bar = dev->bars + i;
	uint32_t barValue = pci_read(dev->bus,dev->dev,dev->func,BAR_OFFSET + i * 4);
	bar->type = barValue & 0x1;
	bar->addr = barValue & ~0xF;
	bar->flags = 0;

	/* to get the size, we set all bits and read it back to see what bits are still set */
	pci_write(dev->bus,dev->dev,dev->func,BAR_OFFSET + i * 4,0xFFFFFFF0 | bar->type);
	bar->size = pci_read(dev->bus,dev->dev,dev->func,BAR_OFFSET + i * 4);
	if(bar->size == 0 || bar->size == 0xFFFFFFFF)
		bar->size = 0;
	else {
		if((bar->size & BAR_ADDR_SPACE) == BAR_ADDR_SPACE_MEM) {
			switch(BAR_MEM_TYPE(barValue)) {
				case 0x00:
					bar->flags |= esc::PCI::Bar::BAR_MEM_32;
					break;
				case 0x02:
					bar->flags |= esc::PCI::Bar::BAR_MEM_64;
					break;
			}
			if(BAR_MEM_PREFETCHABLE(barValue))
				bar->flags |= esc::PCI::Bar::BAR_MEM_PREFETCH;
			bar->size &= BAR_ADDR_MEM_MASK;
		}
		else
			bar->size &= BAR_ADDR_IO_MASK;
		bar->size &= ~(bar->size - 1);
	}
	pci_write(dev->bus,dev->dev,dev->func,BAR_OFFSET + i * 4,barValue);
}
