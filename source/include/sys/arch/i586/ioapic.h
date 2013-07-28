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

#include <sys/common.h>

class OStream;

class IOAPIC {
	IOAPIC() = delete;

	struct Instance {
		uint8_t id;
		uint8_t version;
		uint32_t *addr;
	};

	static const size_t RED_COUNT		= 24;
	static const size_t MAX_IOAPICS		= 8;
	static const size_t IOREGSEL	 	= 0x0 / sizeof(uint32_t);
	static const size_t IOWIN			= 0x10 / sizeof(uint32_t);

	enum {
		IOAPIC_REG_ID		= 0x0,
		IOAPIC_REG_VERSION	= 0x1,
		IOAPIC_REG_ARB		= 0x2,
		IOAPIC_REG_REDTBL	= 0x10
	};

	enum {
		RED_DESTMODE_PHYS	= 0 << 11,
		RED_DESTMODE_LOG	= 1 << 11,
		RED_INT_MASK_DIS	= 0 << 16,
		RED_INT_MASK_EN		= 1 << 16
	};

public:
	static void add(uint8_t id,uint8_t version,uintptr_t addr);
	static void setRedirection(uint8_t dstApic,uint8_t srcIRQ,uint8_t dstInt,uint8_t type,
			uint8_t polarity,uint8_t triggerMode);
	static void print(OStream &os);

private:
	static Instance *get(uint8_t id);
	static uint32_t read(Instance *ioapic,uint32_t reg) {
		ioapic->addr[IOREGSEL] = reg;
		return ioapic->addr[IOWIN];
	}
	static void write(Instance *ioapic,uint32_t reg,uint32_t value) {
		ioapic->addr[IOREGSEL] = reg;
		ioapic->addr[IOWIN] = value;
	}

	static size_t count;
	static Instance ioapics[MAX_IOAPICS];
};
