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
#include <esc/arch/x86/ports.h>

#define IOPORT_PCI_CFG_DATA			0xCFC
#define IOPORT_PCI_CFG_ADDR			0xCF8

static inline uint32_t pci_read(uchar bus,uchar dev,uchar func,uchar offset) {
	uint32_t addr = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC);
	outdword(IOPORT_PCI_CFG_ADDR,addr);
	return indword(IOPORT_PCI_CFG_DATA);
}

static inline void pci_write(uchar bus,uchar dev,uchar func,uchar offset,uint32_t value) {
	uint32_t addr = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC);
	outdword(IOPORT_PCI_CFG_ADDR,addr);
	outdword(IOPORT_PCI_CFG_DATA,value);
}
