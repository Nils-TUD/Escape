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

#include <sys/common.h>
#include <sys/arch/i586/acpi.h>
#include <sys/arch/i586/apic.h>
#include <sys/task/smp.h>
#include <sys/mem/paging.h>
#include <sys/log.h>
#include <sys/util.h>

#define BDA_EBDA			0x40E	/* TODO not always available? */
#define BIOS_AREA			0xE0000
#define SIG(A,B,C,D)		(A + (B << 8) + (C << 16) + (D << 24))

/* root system descriptor pointer */
typedef struct {
	uint32_t signature[2];
	uint8_t checksum;
	char oemId[6];
	uint8_t revision;
	uint32_t rsdtAddr;
	/* since 2.0 */
	uint32_t length;
	uint64_t xsdtAddr;
	uint8_t xchecksum;
} A_PACKED sRSDP;

/* root system descriptor table */
typedef struct {
    uint32_t signature;
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oemId[6];
	char oemTableId[8];
	uint32_t oemRevision;
	char creatorId[4];
	uint32_t creatorRevision;
} A_PACKED sRSDT;

/* APIC entry in RSDT */
typedef struct {
	uint8_t type;
	uint8_t length;
} A_PACKED sAPIC;

/* types of APIC-entries */
enum {
	LAPIC = 0, IOAPIC = 1, INTR = 2,
};

/* special APIC-entry: Local APIC */
typedef struct {
	sAPIC head;
	uint8_t cpu;
	uint8_t id;
	uint32_t flags;
} A_PACKED sLAPIC;

/* special RSDT: for APIC */
typedef struct {
	sRSDT head;
    uint32_t apic_addr;
    uint32_t flags;
    sAPIC apics[];
} A_PACKED sRSDTAPIC;

static bool acpi_sigValid(const sRSDP *rsdp);
static bool acpi_checksumValid(const void *r,size_t len);
static sRSDP *acpi_findIn(uintptr_t start,size_t len);

static sRSDP *rsdp;

bool acpi_find(void) {
	/* first kb of extended bios data area (EBDA) */
	uint16_t ebda = *(uint16_t*)(KERNEL_AREA | BDA_EBDA);
	if((rsdp = acpi_findIn(KERNEL_AREA | ebda * 16,1024)))
		return true;
	/* main bios area below 1 mb */
	if((rsdp = acpi_findIn(KERNEL_AREA | BIOS_AREA,0x20000)))
		return true;
	return false;
}

void acpi_parse(void) {
	sRSDT *rsdt = (sRSDT*)rsdp->rsdtAddr;
	size_t size = sizeof(uint32_t);
	/* check for extended system descriptor table */
	if(rsdp->revision > 1 && acpi_checksumValid(rsdp,rsdp->length)) {
		rsdt = (sRSDT*)rsdp->xsdtAddr;
		size = sizeof(uint64_t);
	}

	/* first map the RSDT. we assume that it covers only one page */
	uintptr_t addr = (uintptr_t)rsdt;
	size_t off = addr & (PAGE_SIZE - 1);
	paging_map(ACPI_AREA,&addr,1,PG_PRESENT | PG_SUPERVISOR | PG_ADDR_TO_FRAME);
	rsdt = (sRSDT*)(ACPI_AREA + off);
	size_t i,count = (rsdt->length - sizeof(sRSDT)) / size;

	/* now walk through the tables behind the RSDT */
	uintptr_t table[count];
	uintptr_t min = 0xFFFFFFFF, max = 0;
	uintptr_t start = (uintptr_t)(rsdt + 1);
	for(i = 0; i < count; i++) {
		if(start + (i + 1) * size > ACPI_AREA + PAGE_SIZE) {
			log_printf("ACPI RSDT pointer @ %p out of bounce (%p..%p). Assuming one CPU.",
					start + i * size,ACPI_AREA,ACPI_AREA + PAGE_SIZE);
			return;
		}
		if(size == sizeof(uint32_t))
			table[i] = *(uint32_t*)(start + i * size);
		else
			table[i] = *(uint64_t*)(start + i * size);
		/* determine the address range for the RSDT's */
		if(table[i] < min)
			min = table[i];
		if(table[i] > max)
			max = table[i];
	}

	/* map the area, if possible */
	size_t rsdtSize = (max + PAGE_SIZE * 2 - 1) - (min & ~(PAGE_SIZE - 1));
	if(rsdtSize > ACPI_AREA_SIZE - PAGE_SIZE) {
		log_printf("ACPI RSDT area too large (need %zu bytes). Assuming one CPU.",rsdtSize);
		return;
	}
	addr = min;
	for(i = 0; i < rsdtSize / PAGE_SIZE; i++, addr += PAGE_SIZE) {
		paging_map(ACPI_AREA + PAGE_SIZE * (i + 1),&addr,1,
				PG_PRESENT | PG_SUPERVISOR | PG_ADDR_TO_FRAME);
	}
	/* change addresses accordingly */
	for(i = 0; i < count; i++)
		table[i] = ACPI_AREA + PAGE_SIZE + (min & (PAGE_SIZE - 1)) + table[i] - min;

	/* walk through the table and search for APIC entries */
	cpuid_t id = apic_getId();
	for(i = 0; i < count; i++) {
		sRSDT *tbl = (sRSDT*)table[i];
		if(acpi_checksumValid(tbl,tbl->length) && tbl->signature == SIG('A','P','I','C')) {
			sRSDTAPIC *rapic = (sRSDTAPIC*)tbl;
			sAPIC *apic = rapic->apics;
			while(apic < (sAPIC*)((uintptr_t)tbl + tbl->length)) {
				/* we're only interested in the local APICs */
				if(apic->type == LAPIC) {
					sLAPIC *lapic = (sLAPIC*)apic;
					if(lapic->flags & 0x1)
						smp_addCPU(lapic->id == id,lapic->id,false);
				}
				apic = (sAPIC*)((uintptr_t)apic + apic->length);
			}
		}
	}
}

static bool acpi_sigValid(const sRSDP *r) {
	return r->signature[0] == SIG('R','S','D',' ') && r->signature[1] == SIG('P','T','R',' ');
}

static bool acpi_checksumValid(const void *r,size_t len) {
	uint8_t sum = 0;
	uint8_t *p;
	for(p = (uint8_t*)r; p < (uint8_t*)r + len; p++)
		sum += *p;
	return sum == 0;
}

static sRSDP *acpi_findIn(uintptr_t start,size_t len) {
	uintptr_t addr;
	for(addr = start; addr < start + len; addr += 16) {
		sRSDP *r = (sRSDP*)addr;
		if(acpi_sigValid(r) && acpi_checksumValid(r,20))
			return r;
	}
	return NULL;
}
