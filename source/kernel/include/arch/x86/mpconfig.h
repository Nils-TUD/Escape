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

#include <common.h>

class MPConfig {
	MPConfig() = delete;

	struct FloatPtr {
		uint32_t signature;
		uint32_t mpConfigTable;
		uint8_t length;
		uint8_t specRev;
		uint8_t checkSum;
		uint8_t feature[5];
	} A_PACKED;

	struct TableHeader {
		uint32_t signature;
		uint16_t baseTableLength;
		uint8_t specRev;
		uint8_t checkSum;
		char oemId[8];
		char productId[12];
		uint32_t oemTablePointer;
		uint16_t oemTableSize;
		uint16_t entryCount;
		uint32_t localAPICAddr;
		uint32_t reserved;
	} A_PACKED;

	struct Proc {
		uint8_t type;
		uint8_t localAPICId;
		uint8_t localAPICVersion;
		uint8_t cpuFlags;
		uint8_t cpuSignature[4];
		uint32_t featureFlags;
		uint32_t reserved[2];
	} A_PACKED;

	struct MPIOAPIC {
		uint8_t type;
		uint8_t ioAPICId;
		uint8_t ioAPICVersion;
		uint8_t enabled : 1,
				: 7;
		uint32_t baseAddr;
	} A_PACKED;

	struct IOIntrptEntry {
		uint8_t type;
		uint8_t intrptType;
		uint16_t : 12,
			triggerMode : 2,
			polarity : 2;
		uint8_t srcBusId;
		uint8_t srcBusIRQ;
		uint8_t dstIOAPICId;
		uint8_t dstIOAPICInt;
	} A_PACKED;

	enum {
		POL_BUS				= 0x0,
		POL_HIGH_ACTIVE		= 0x1,
		POL_LOW_ACTIVE		= 0x3,
	};
	enum {
		TRIG_BUS			= 0x0,
		TRIG_EDGE			= 0x1,
		TRIG_LEVEL			= 0x3,
	};
	enum {
		TYPE_INT			= 0,
		TYPE_NMI			= 1,
		TYPE_SMI			= 2,
		TYPE_EXTINT			= 3,
	};

	struct LocalIntrptEntry {
		uint8_t type;
		uint8_t intrptType;
		uint16_t : 12,
			triggerMode : 2,
			polarity : 2;
		uint8_t srcBusId;
		uint8_t srcBusIRQ;
		uint8_t dstLAPICId;
		uint8_t dstLAPICInt;
	} A_PACKED;

	enum {
		MPCTE_TYPE_PROC		 = 0,
		MPCTE_LEN_PROC		 = 20,
		MPCTE_TYPE_BUS		 = 1,
		MPCTE_LEN_BUS		 = 8,
		MPCTE_TYPE_IOAPIC	 = 2,
		MPCTE_LEN_IOAPIC	 = 8,
		MPCTE_TYPE_IOIRQ	 = 3,
		MPCTE_LEN_IOIRQ		 = 8,
		MPCTE_TYPE_LIRQ		 = 4,
		MPCTE_LEN_LIRQ		 = 8,
	};

public:
	/**
	 * Searches for the multiprocessor configuration
	 *
	 * @return true if found
	 */
	static bool find();

	/**
	 * Parses the multiprocessor configuration and adds found CPUs to the SMP-module
	 */
	static void parse();

private:
	static void handleIOInt(IOIntrptEntry *ioint);
	static FloatPtr *search();
	static FloatPtr *searchIn(uintptr_t start,size_t length);

	static FloatPtr *mpf;
};
