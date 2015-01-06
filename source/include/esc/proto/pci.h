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
#include <sys/messages.h>
#include <esc/proto/default.h>
#include <esc/vthrow.h>

namespace esc {

/**
 * The IPC-interface for the PCI device. Allows you to find PCI devices by BDF, class etc..
 */
class PCI {
public:
	struct Bar {
		enum {
			BAR_MEM				= 0,
			BAR_IO				= 1
		};
		enum {
			BAR_MEM_32			= 0x1,
			BAR_MEM_64			= 0x2,
			BAR_MEM_PREFETCH	= 0x4
		};

		uint type;
		uintptr_t addr;
		size_t size;
		uint flags;
	};

	enum DevType {
		GENERIC					= 0,
		PCI_PCI_BRIDGE			= 1,
		CARD_BUS_BRIDGE			= 2,
	};

	enum Status {
		ST_IRQ					= 1 << 3,
		ST_CAPS					= 1 << 4,
	};

	enum Cap {
		CAP_PM					= 0x01,	/* Power Management */
		CAP_AGP					= 0x02,	/* Accelerated Graphics Port */
		CAP_VPD					= 0x03,	/* Vital Product Data */
		CAP_SLOTID				= 0x04,	/* Slot Identification */
		CAP_MSI					= 0x05,	/* Message Signalled Interrupts */
		CAP_CHSWP				= 0x06,	/* CompactPCI HotSwap */
		CAP_PCIX				= 0x07,	/* PCI-X */
		CAP_HT					= 0x08,	/* HyperTransport */
		CAP_VNDR				= 0x09,	/* Vendor specific */
		CAP_DBG					= 0x0A,	/* Debug port */
		CAP_CCRC				= 0x0B,	/* CompactPCI Central Resource Control */
		CAP_SHPC 				= 0x0C,	/* PCI Standard Hot-Plug Controller */
		CAP_SSVID				= 0x0D,	/* Bridge subsystem vendor/device ID */
		CAP_AGP3				= 0x0E,	/* AGP Target PCI-PCI bridge */
		CAP_SECDEV				= 0x0F,	/* Secure Device */
		CAP_EXP 				= 0x10,	/* PCI Express */
		CAP_MSIX				= 0x11,	/* MSI-X */
		CAP_SATA				= 0x12,	/* SATA Data/Index Conf. */
		CAP_AF					= 0x13,	/* PCI Advanced Features */
	};

	struct Device {
		uchar bus;
		uchar dev;
		uchar func;
		uchar type;
		ushort deviceId;
		ushort vendorId;
		uchar baseClass;
		uchar subClass;
		uchar progInterface;
		uchar revId;
		uchar irq;
		Bar bars[6];
	};

	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the operation failed
	 */
	explicit PCI(const char *path) : _is(path) {
	}

	/**
	 * Finds the device with given class and subclass.
	 *
	 * @param cls the class
	 * @param subcls the subclass
	 * @param no the number (if there are multiple devices with this class/subclass)
	 * @return the device
	 * @throws if the operation failed
	 */
	Device getByClass(uchar cls,uchar subcls,int no = 0) {
		Device d;
		int err;
		_is << cls << subcls << no << SendReceive(MSG_PCI_GET_BY_CLASS) >> err >> d;
		if(err < 0)
			VTHROWE("getByClass(" << cls << "," << subcls << ")",err);
		return d;
	}

	/**
	 * Finds the device with given BDF.
	 *
	 * @param bus the bus
	 * @param dev the device
	 * @param func the function
	 * @return the device
	 * @throws if the operation failed
	 */
	Device getById(uchar bus,uchar dev,uchar func) {
		Device d;
		int err;
		_is << bus << dev << func << SendReceive(MSG_PCI_GET_BY_ID) >> err >> d;
		if(err < 0)
			VTHROWE("getById(" << bus << ":" << dev << ":" << func << ")",err);
		return d;
	}

	/**
	 * Finds the device with given index (0...n-1).
	 *
	 * @param idx the index
	 * @return the device
	 * @throws if the operation failed
	 */
	Device getByIndex(size_t idx) {
		Device d;
		int err;
		_is << idx << SendReceive(MSG_PCI_GET_BY_INDEX) >> err >> d;
		if(err < 0)
			VTHROWE("getByIndex(" << idx << ")",err);
		return d;
	}

	/**
	 * Determines the total number of PCI devices.
	 *
	 * @return the count
	 * @throws if the operation failed
	 */
	size_t getCount() {
		ssize_t count;
		_is << SendReceive(MSG_PCI_GET_COUNT) >> count;
		if(count < 0)
			VTHROWE("getCount()",count);
		return count;
	}

	/**
	 * Reads from the PCI config space of the given device.
	 *
	 * @param bus the bus
	 * @param dev the device
	 * @param func the function
	 * @param offset the offset to read at
	 * @return the value
	 */
	uint32_t read(uchar bus,uchar dev,uchar func,uint32_t offset) {
		uint32_t res;
		_is << bus << dev << func << offset << SendReceive(MSG_PCI_READ) >> res;
		return res;
	}

	/**
	 * Writes to the PCI config space of the given device.
	 *
	 * @param bus the bus
	 * @param dev the device
	 * @param func the function
	 * @param offset the offset to write to
	 * @param value the value to write
	 */
	void write(uchar bus,uchar dev,uchar func,uint32_t offset,uint32_t value) {
		_is << bus << dev << func << offset << value << Send(MSG_PCI_WRITE);
	}

	/**
	 * Determines whether the given capability exists.
	 *
	 * @param bus the bus
	 * @param dev the device
	 * @param func the function
	 * @param id the capability id
	 * @return true if found
	 */
	bool hasCap(uchar bus,uchar dev,uchar func,uint8_t id) {
		bool res;
		_is << bus << dev << func << id << SendReceive(MSG_PCI_HAS_CAP) >> res;
		return res;
	}

	/**
	 * Enables MSIs for the given PCI device.
	 *
	 * @param bus the bus
	 * @param dev the device
	 * @param func the function
	 * @param msiaddr the MSI address to program
	 * @param msival the MSI value to program
	 * @return 0 on success
	 */
	uint8_t enableMSIs(uchar bus,uchar dev,uchar func,uint64_t msiaddr,uint32_t msival) {
		int res;
		_is << bus << dev << func << msiaddr << msival << SendReceive(MSG_PCI_ENABLE_MSIS) >> res;
		return res;
	}

private:
	IPCStream _is;
};

}
