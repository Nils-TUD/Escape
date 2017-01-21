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

#include <esc/ipc/device.h>
#include <esc/proto/pci.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/driver.h>
#include <sys/messages.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "pci.h"

#define DEBUG	0

#if DEBUG
#	define DBG(fmt...)		print(fmt)
#else
#	define DBG(...)
#endif

using namespace esc;

class PCIService : public Device {
public:
	explicit PCIService(const char *name,mode_t mode)
		: Device(name,mode,DEV_TYPE_SERVICE,DEV_CLOSE) {
		set(MSG_PCI_GET_BY_CLASS,std::make_memfun(this,&PCIService::getByClass));
		set(MSG_PCI_GET_BY_ID,std::make_memfun(this,&PCIService::getById));
		set(MSG_PCI_GET_BY_INDEX,std::make_memfun(this,&PCIService::getByIndex));
		set(MSG_PCI_GET_COUNT,std::make_memfun(this,&PCIService::getCount));
		set(MSG_PCI_READ,std::make_memfun(this,&PCIService::read));
		set(MSG_PCI_WRITE,std::make_memfun(this,&PCIService::write));
		set(MSG_PCI_HAS_CAP,std::make_memfun(this,&PCIService::hasCap));
		set(MSG_PCI_ENABLE_MSIS,std::make_memfun(this,&PCIService::enableMSIs));
	}

	void getByClass(IPCStream &is) {
		int no;
		uchar cls,subcls;
		is >> cls >> subcls >> no;

		PCI::Device *d = list_getByClass(cls,subcls,no);
		replyDevice(is,d);
	}

	void getById(IPCStream &is) {
		uchar bus,dev,func;
		is >> bus >> dev >> func;

		PCI::Device *d = list_getById(bus,dev,func);
		replyDevice(is,d);
	}

	void getByIndex(IPCStream &is) {
		size_t idx;
		is >> idx;

		PCI::Device *d = list_get(idx);
		replyDevice(is,d);
	}

	void getCount(IPCStream &is) {
		size_t len = list_length();
		is << ValueResponse<size_t>::success(len) << Reply();
	}

	void read(IPCStream &is) {
		uchar bus,dev,func;
		uint32_t offset,value;
		is >> bus >> dev >> func >> offset;

		value = pci_read(bus,dev,func,offset);
		DBG("%02x:%02x:%x [%#04x] -> %#08x",bus,dev,func,offset,value);
		is << ValueResponse<uint32_t>::success(value) << Reply();
	}

	void write(IPCStream &is) {
		uchar bus,dev,func;
		uint32_t offset,value;
		is >> bus >> dev >> func >> offset >> value;

		DBG("%02x:%02x:%x [%#04x] <- %#08x",bus,dev,func,offset,value);
		pci_write(bus,dev,func,offset,value);
		is << errcode_t(0) << Reply();
	}

	void hasCap(IPCStream &is) {
		uchar bus,dev,func,cap;
		is >> bus >> dev >> func >> cap;

		uint8_t offset = getCapOff(bus,dev,func,cap);
		DBG("%02x:%02x:%x CAP %#02x -> %#02x",bus,dev,func,cap,offset);
		is << ValueResponse<bool>::success(offset != 0) << Reply();
	}

	void enableMSIs(IPCStream &is) {
		uchar bus,dev,func;
		uint64_t msiaddr;
		uint32_t msival;
		is >> bus >> dev >> func >> msiaddr >> msival;

		uint8_t offset = getCapOff(bus,dev,func,PCI::CAP_MSI);
		if(offset) {
			uint32_t ctrl = pci_read(bus,dev,func,offset);
			size_t base = offset + 4;
			pci_write(bus,dev,func,base + 0,msiaddr & 0xFFFFFFFF);
			pci_write(bus,dev,func,base + 4,msiaddr >> 32);
			if(ctrl & 0x800000)
				base += 4;
			pci_write(bus,dev,func,base + 4,msival);

			// we use only a single message and enable MSIs here
			pci_write(bus,dev,func,offset,(ctrl & ~0x700000) | 0x10000);

			DBG("%02x:%02x:%x enabled MSIs %#08Lx : %#08x",bus,dev,func,msiaddr,msival);
		}

		is << errcode_t(offset ? 0 : -EINVAL) << Reply();
	}

private:
	static uint8_t getCapOff(uchar bus,uchar dev,uchar func,uint8_t cap) {
		uint32_t statusCmd = pci_read(bus,dev,func,0x04);
		if((statusCmd >> 16) & PCI::ST_CAPS) {
			uint8_t offset = pci_read(bus,dev,func,0x34);
			while((offset != 0) && !(offset & 0x3)) {
				uint32_t val = pci_read(bus,dev,func,offset);
				if((val & 0xFF) == cap)
					return offset;
				offset = val >> 8;
			}
		}
		return 0;
	}

	void replyDevice(IPCStream &is,PCI::Device *d) {
		if(d)
			is << ValueResponse<PCI::Device>::success(*d) << Reply();
		else
			is << ValueResponse<PCI::Device>::error(-ENOTFOUND) << Reply();
	}
};

int main(void) {
	list_init();

	PCIService dev("/dev/pci",0110);
	dev.loop();
	return EXIT_SUCCESS;
}
