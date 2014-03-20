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
#include <esc/driver.h>
#include <esc/debug.h>
#include <esc/messages.h>
#include <ipc/device.h>
#include <ipc/proto/pci.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"

using namespace ipc;

class PCIService : public Device {
public:
	explicit PCIService(const char *name,mode_t mode)
		: Device(name,mode,DEV_TYPE_SERVICE,DEV_CLOSE) {
		set(MSG_PCI_GET_BY_CLASS,std::make_memfun(this,&PCIService::getByClass));
		set(MSG_PCI_GET_BY_ID,std::make_memfun(this,&PCIService::getById));
		set(MSG_PCI_GET_BY_INDEX,std::make_memfun(this,&PCIService::getByIndex));
		set(MSG_PCI_GET_COUNT,std::make_memfun(this,&PCIService::getCount));
	}

	void getByClass(IPCStream &is) {
		uchar cls,subcls;
		is >> cls >> subcls;

		PCI::Device *d = list_getByClass(cls,subcls);
		int res = d ? 0 : -1;
		is << res << *d << Send(PCI::RES_MID);
	}

	void getById(IPCStream &is) {
		uchar bus,dev,func;
		is >> bus >> dev >> func;

		PCI::Device *d = list_getById(bus,dev,func);
		int res = d ? 0 : -1;
		is << res << *d << Send(PCI::RES_MID);
	}

	void getByIndex(IPCStream &is) {
		size_t idx;
		is >> idx;

		PCI::Device *d = list_get(idx);
		int res = d ? 0 : -1;
		is << res << *d << Send(PCI::RES_MID);
	}

	void getCount(IPCStream &is) {
		ssize_t len = list_length();
		is << len << Send(PCI::RES_MID);
	}
};

int main(void) {
	list_init();

	PCIService dev("/dev/pci",0111);
	dev.loop();
	return EXIT_SUCCESS;
}
