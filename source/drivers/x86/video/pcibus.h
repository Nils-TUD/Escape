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

#pragma once

#include <sys/common.h>
#include <ipc/proto/pci.h>
#include <stdio.h>

class PCIBus {
	enum {
		PCI_ADDR_REG = 0xCF8,
		PCI_DATA_REG = 0xCFC
	};

public:
	explicit PCIBus() : _pci("/dev/pci"), _dev(), _addr(), _addrValid() {
		try {
			_dev = _pci.getByClass(0x03,0x00);
		}
		catch(...) {
		}
	}

	template <typename T>
	bool write(unsigned short port,T val) {
		switch(port) {
			case PCI_ADDR_REG: {
				if(sizeof(T) != 4) {
					print("writing with size %zu not supported",sizeof(T));
					return true;
				}

				unsigned devbdf = (val >> 8) & 0xFFFF;
				unsigned vgabdf = (_dev.bus << 8) | (_dev.dev << 3) | _dev.func;
				if(devbdf != vgabdf) {
					_addrValid = false;
					return true;
				}

				_addrValid = true;
				_addr = val & 0xFC;
				return true;
			}

			case PCI_DATA_REG:
				print("writing data register not supported (value=%#x)",(int)val);
				return true;

			default:
				return false;
		}
	}

	template <typename T>
	bool read(unsigned short port,T *val) {
		unsigned raw_val;
		*val = 0;

		switch(port & ~3) {
			case PCI_ADDR_REG:
				*val = _addr;
				return true;

			case PCI_DATA_REG: {
				if(!_addrValid) {
					*val = 0;
					return true;
				}

				switch(_addr) {
					case 0:		/* vendor / device ID */
						raw_val = _dev.vendorId | (_dev.deviceId << 16);
						break;

					case 4:		/* status and command */
					case 8:		/* class code / revision ID */
					case 0x10:	/* base address register 0 */
					case 0x14:	/* base address register 1 */
					case 0x18:	/* base address register 2 */
					case 0x1c:	/* base address register 3 */
					case 0x20:	/* base address register 4 */
					case 0x24:	/* base address register 5 */
						raw_val = _pci.read(_dev.bus,_dev.dev,_dev.func,_addr);
						break;

					default:
						print("unexpected configuration address %p",_addr);
						return true;
				}

				*val = raw_val >> ((port & 3) * 8);
				return true;
			}

			default:
				return false;
		}
	}

private:
	ipc::PCI _pci;
	ipc::PCI::Device _dev;
	uintptr_t _addr;
	bool _addrValid;
};
