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
#include <esc/sllist.h>

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

	/* root system descriptor table */
	struct RSDT {
	    uint32_t signature;
		uint32_t length;
		uint8_t revision;
		uint8_t checksum;
		char oemId[6];
		char oemTableId[8];
		uint32_t oemRevision;
		char creatorId[4];
		uint32_t creatorRevision;
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

	/* special APIC-entry: Local APIC */
	struct LAPIC {
		APIC head;
		uint8_t cpu;
		uint8_t id;
		uint32_t flags;
	} A_PACKED;

	/* special RSDT: for APIC */
	struct RSDTAPIC {
		RSDT head;
	    uint32_t apic_addr;
	    uint32_t flags;
	    APIC apics[];
	} A_PACKED;

public:
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
	/**
	 * Prints all ACPI tables.
	 */
	static void print();

private:
	static bool sigValid(const RSDP *rsdp);
	static bool checksumValid(const void *r,size_t len);
	static RSDP *findIn(uintptr_t start,size_t len);

	static RSDP *rsdp;
	static sSLList acpiTables;
};
