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
#include <sys/col/islist.h>
#include <esc/arch/i586/acpi.h>

class OStream;

class ACPI {
	ACPI() = delete;

	/* root system descriptor pointer */
	struct RSDP {
		uint32_t signature[2];
		uint8_t checksum;
		char oemId[6];
		uint8_t revision;
		uint32_t rsdtAddr;
		/* since 2.0 */
		uint32_t length;
		uint64_t xsdtAddr;
		uint8_t xchecksum;
	} A_PACKED;

	/* special RSDT: FACP */
	struct RSDTFACP {
		sRSDT head;
		uint32_t :32;
		uint32_t DSDT;
	} A_PACKED;

	/* APIC entry in RSDT */
	struct APIC {
		uint8_t type;
		uint8_t length;
	} A_PACKED;

	/* types of APIC-entries */
	enum {
		TYPE_LAPIC	= 0,
		TYPE_IOAPIC	= 1,
		TYPE_INTR	= 2,
	};

	/* special RSDT: for APIC */
	struct RSDTAPIC {
		sRSDT head;
	    uint32_t apic_addr;
	    uint32_t flags;
	    APIC apics[];
	} A_PACKED;

	/* special APIC-entry: Local APIC */
	struct APICLAPIC {
		APIC head;
		uint8_t cpu;
		uint8_t id;
		uint32_t flags;
	} A_PACKED;

	/* special APIC-entry: I/O APIC */
	struct APICIOAPIC {
		APIC head;
		uint8_t id;
		uint8_t : 8;
		uint32_t address;
		uint32_t baseGSI;
	} A_PACKED;

	/* special APIC-entry: Interrupt Source Override */
	struct APICIntSO {
		APIC head;
		uint8_t bus;
		uint8_t source;
		uint32_t gsi;
		uint16_t flags;
	} A_PACKED;

	enum {
		INTI_POL_MASK			= 0x3,
		INTI_POL_BUS			= 0x0,
		INTI_POL_HIGH_ACTIVE	= 0x1,
		INTI_POL_LOW_ACTIVE		= 0x3,
	};
	enum {
		INTI_TRIGGER_MASK		= 0xC,
		INTI_TRIG_BUS			= 0x0,
		INTI_TRIG_EDGE			= 0x4,
		INTI_TRIG_LEVEL			= 0xC,
	};

public:
	/**
	 * Inits ACPI, i.e. finds the root system description pointer and parses the tables.
	 */
	static void init();
	/**
	 * @return true if ACPI is available
	 */
	static bool isEnabled() {
		return enabled;
	}
	/**
	 * Creates files in the VFS for all ACPI tables
	 */
	static void create_files();
	/**
	 * Prints all ACPI tables.
	 *
	 * @param os the output-stream
	 */
	static void print(OStream &os);

private:
	/**
	 * Searches for the root system description pointer.
	 *
	 * @return true if found
	 */
	static bool find();
	/**
	 * Parses the ACPI tables, stores them in a linked list and adds the found CPUs to the CPU-list
	 * in the SMP-module.
	 */
	static void parse();
	static void addTable(sRSDT *tbl,size_t i,uintptr_t &curDest,uintptr_t destEnd);
	static bool sigValid(const RSDP *rsdp);
	static RSDP *findIn(uintptr_t start,size_t len);

	static bool enabled;
	static RSDP *rsdp;
	static ISList<sRSDT*> acpiTables;
};
