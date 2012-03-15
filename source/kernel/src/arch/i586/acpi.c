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
#include <sys/mem/paging.h>

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

static bool acpi_sigValid(sRSDP *rsdp);
static bool acpi_checksumValid(sRSDP *r,size_t len);
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
	uintptr_t addr = rsdp->rsdtAddr;
	size_t size = sizeof(uint32_t);
	/* check for extended system descriptor table */
	if(rsdp->revision > 1 && acpi_checksumValid(rsdp,rsdp->length)) {
		addr = rsdp->xsdtAddr;
		size = sizeof(uint64_t);
	}

	size_t i,count = (rsdp->length - sizeof(sRSDT)) / size;
	uintptr_t table[count];
	for(i = 0; i < count; i++) {
		if(size == sizeof(uint32_t))
			table[i] = *(uint32_t*)(addr + i * size);
		else
			table[i] = *(uint64_t*)(addr + i * size);
	}/*

	for(i = 0; i < count; i++) {

	}*/
}

static bool acpi_sigValid(sRSDP *r) {
	return r->signature[0] == SIG('R','S','D',' ') && r->signature[1] == SIG('P','T','R',' ');
}

static bool acpi_checksumValid(sRSDP *r,size_t len) {
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
