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

#pragma once

#include <esc/common.h>
#include <esc/arch/i586/acpi.h>
#include "../../machine.h"

class i586Machine : public Machine {
private:
	static const uint16_t PORT_KB_DATA	= 0x60;
	static const uint16_t PORT_KB_CTRL	= 0x64;

	enum FACPFlags {
		RESET_REG_SUP	= 1 << 10
	};

	enum AddressSpace {
		SYS_MEM			= 0,
		SYS_IO			= 1,
		PCI_CONF_SPACE	= 2,
		EMBEDDED_CTRL	= 3,
		SMBUS			= 4,
	};

	struct GAS {
		uint8_t addressSpace;
		uint8_t regBitWidth;
		uint8_t regBitOffset;
		uint8_t accessSize;
		uint64_t address;
	} A_PACKED;

	struct FACP {
		sRSDT head;
		uint32_t :32;
		uint32_t DSDT;
		uint32_t :32;
		uint32_t SMI_CMD;
		uint8_t ACPI_ENABLE;
		uint8_t ACPI_DISABLE;
		uint32_t : 32;
		uint32_t : 32;
		uint16_t : 16;
		uint32_t PM1a_CNT_BLK;
		uint32_t PM1b_CNT_BLK;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t flags;
		GAS RESET_REG;
		uint8_t RESET_VALUE;
	} A_PACKED;

	struct ShutdownInfo {
		uint PM1a_CNT;
		uint PM1b_CNT;
		uint SLP_TYPa;
		uint SLP_TYPb;
		uint SLP_EN;
	};

public:
	i586Machine()
		: Machine() {
	}
	virtual ~i586Machine() {
	}

	virtual void reboot(Progress &pg);
	virtual void shutdown(Progress &pg);

private:
	void rebootPCI();
	void rebootSysCtrlPort();
	void rebootACPI();
	void rebootPulseResetLine();
	void *mapTable(const char *name,size_t *len);
	bool shutdownSupported(ShutdownInfo *info);
};
