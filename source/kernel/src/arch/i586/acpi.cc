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
#include <sys/mem/sllnodes.h>
#include <sys/log.h>
#include <sys/util.h>
#include <sys/video.h>
#include <string.h>

#define MAX_ACPI_PAGES		16
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
static sSLList acpiTables;

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

	sll_init(&acpiTables,slln_allocNode,slln_freeNode);

	/* first map the RSDT. we assume that it covers only one page */
	size_t off = (uintptr_t)rsdt & (PAGE_SIZE - 1);
	rsdt = (sRSDT*)(paging_makeAccessible((uintptr_t)rsdt,1) + off);
	size_t i,j,count = (rsdt->length - sizeof(sRSDT)) / size;
	if(off + rsdt->length > PAGE_SIZE) {
		size_t pages = BYTES_2_PAGES(off + rsdt->length);
		rsdt = (sRSDT*)(paging_makeAccessible((uintptr_t)rsdt,pages) + off);
	}

	/* now walk through the tables behind the RSDT */
	uintptr_t curDest = paging_makeAccessible(0,MAX_ACPI_PAGES);
	uintptr_t destEnd = curDest + MAX_ACPI_PAGES * PAGE_SIZE;
	uintptr_t start = (uintptr_t)(rsdt + 1);
	for(i = 0; i < count; i++) {
		sRSDT *tbl;
		if(size == sizeof(uint32_t))
			tbl = (sRSDT*)*(uint32_t*)(start + i * size);
		else
			tbl = (sRSDT*)*(uint64_t*)(start + i * size);

		/* map it to a temporary place; try one page first and extend it if necessary */
		size_t tmpPages = 1;
		size_t tbloff = (uintptr_t)tbl & (PAGE_SIZE - 1);
		frameno_t frm = (uintptr_t)tbl / PAGE_SIZE;
		uintptr_t tmp = paging_mapToTemp(&frm,1);
		sRSDT *tmptbl = (sRSDT*)(tmp + tbloff);
		if((uintptr_t)tmptbl + tmptbl->length > tmp + PAGE_SIZE) {
			size_t tbllen = tmptbl->length;
			paging_unmapFromTemp(1);
			/* determine the real number of required pages */
			tmpPages = (tbloff + tbllen + PAGE_SIZE - 1) / PAGE_SIZE;
			if(tmpPages > TEMP_MAP_AREA_SIZE / PAGE_SIZE) {
				log_printf("Skipping ACPI table %zu (too large: %zu)\n",i,tbllen);
				continue;
			}
			/* map it again */
			frameno_t framenos[tmpPages];
			for(j = 0; j < tmpPages; ++j)
				framenos[j] = ((uintptr_t)tbl + j * PAGE_SIZE) / PAGE_SIZE;
			tmp  = paging_mapToTemp(framenos,tmpPages);
			tmptbl = (sRSDT*)(tmp + tbloff);
		}

		/* do we have to extend the mapping in the ACPI-area? */
		if(curDest + tmptbl->length > destEnd) {
			log_printf("Skipping ACPI table %zu (doesn't fit anymore: %p vs. %p)\n",i,
					curDest + tmptbl->length,destEnd);
			paging_unmapFromTemp(tmpPages);
			continue;
		}

		if(acpi_checksumValid(tmptbl,tmptbl->length)) {
			/* copy the table */
			memcpy((void*)curDest,tmptbl,tmptbl->length);
			sll_append(&acpiTables,(void*)curDest);
			curDest += tmptbl->length;
		}
		else
			log_printf("Checksum of table %zu is invalid. Skipping\n",i);
		paging_unmapFromTemp(tmpPages);
	}

	/* walk through the table and search for APIC entries */
	cpuid_t id = apic_getId();
	sSLNode *n;
	for(n = sll_begin(&acpiTables); n != NULL; n = n->next) {
		sRSDT *tbl = (sRSDT*)n->data;
		if(acpi_checksumValid(tbl,tbl->length) && tbl->signature == SIG('A','P','I','C')) {
			sRSDTAPIC *rapic = (sRSDTAPIC*)tbl;
			sAPIC *apic = rapic->apics;
			while(apic < (sAPIC*)((uintptr_t)tbl + tbl->length)) {
				/* we're only interested in the local APICs */
				if(apic->type == LAPIC) {
					sLAPIC *lapic = (sLAPIC*)apic;
					if(lapic->flags & 0x1)
						SMP::addCPU(lapic->id == id,lapic->id,false);
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

void acpi_print(void) {
	sSLNode *n;
	size_t i = 0;
	vid_printf("ACPI tables:\n");
	for(n = sll_begin(&acpiTables); n != NULL; n = n->next, i++) {
		sRSDT *tbl = (sRSDT*)n->data;
		vid_printf("\tTable%zu:\n",i);
		vid_printf("\t\tsignature: %.4s\n",(char*)&tbl->signature);
		vid_printf("\t\tlength: %u\n",tbl->length);
		vid_printf("\t\trevision: %u\n",tbl->revision);
		vid_printf("\t\toemId: %.6s\n",tbl->oemId);
		vid_printf("\t\toemTableId: %.8s\n",tbl->oemTableId);
		vid_printf("\n");
	}
}
