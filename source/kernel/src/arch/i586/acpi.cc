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
#include <sys/arch/i586/lapic.h>
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

ACPI::RSDP *ACPI::rsdp;
sSLList ACPI::acpiTables;

bool ACPI::find() {
	/* first kb of extended bios data area (EBDA) */
	uint16_t ebda = *(uint16_t*)(KERNEL_AREA | BDA_EBDA);
	if((rsdp = findIn(KERNEL_AREA | ebda * 16,1024)))
		return true;
	/* main bios area below 1 mb */
	if((rsdp = findIn(KERNEL_AREA | BIOS_AREA,0x20000)))
		return true;
	return false;
}

void ACPI::parse() {
	RSDT *rsdt = (RSDT*)rsdp->rsdtAddr;
	size_t size = sizeof(uint32_t);
	/* check for extended system descriptor table */
	if(rsdp->revision > 1 && checksumValid(rsdp,rsdp->length)) {
		rsdt = (RSDT*)rsdp->xsdtAddr;
		size = sizeof(uint64_t);
	}

	sll_init(&acpiTables,slln_allocNode,slln_freeNode);

	/* first map the RSDT. we assume that it covers only one page */
	size_t off = (uintptr_t)rsdt & (PAGE_SIZE - 1);
	rsdt = (RSDT*)(PageDir::makeAccessible((uintptr_t)rsdt,1) + off);
	size_t i,j,count = (rsdt->length - sizeof(RSDT)) / size;
	if(off + rsdt->length > PAGE_SIZE) {
		size_t pages = BYTES_2_PAGES(off + rsdt->length);
		rsdt = (RSDT*)(PageDir::makeAccessible((uintptr_t)rsdt,pages) + off);
	}

	/* now walk through the tables behind the RSDT */
	uintptr_t curDest = PageDir::makeAccessible(0,MAX_ACPI_PAGES);
	uintptr_t destEnd = curDest + MAX_ACPI_PAGES * PAGE_SIZE;
	uintptr_t start = (uintptr_t)(rsdt + 1);
	for(i = 0; i < count; i++) {
		RSDT *tbl;
		if(size == sizeof(uint32_t))
			tbl = (RSDT*)*(uint32_t*)(start + i * size);
		else
			tbl = (RSDT*)*(uint64_t*)(start + i * size);

		/* map it to a temporary place; try one page first and extend it if necessary */
		size_t tmpPages = 1;
		size_t tbloff = (uintptr_t)tbl & (PAGE_SIZE - 1);
		frameno_t frm = (uintptr_t)tbl / PAGE_SIZE;
		uintptr_t tmp = PageDir::mapToTemp(&frm,1);
		RSDT *tmptbl = (RSDT*)(tmp + tbloff);
		if((uintptr_t)tmptbl + tmptbl->length > tmp + PAGE_SIZE) {
			size_t tbllen = tmptbl->length;
			PageDir::unmapFromTemp(1);
			/* determine the real number of required pages */
			tmpPages = (tbloff + tbllen + PAGE_SIZE - 1) / PAGE_SIZE;
			if(tmpPages > TEMP_MAP_AREA_SIZE / PAGE_SIZE) {
				Log::printf("Skipping ACPI table %zu (too large: %zu)\n",i,tbllen);
				continue;
			}
			/* map it again */
			frameno_t framenos[tmpPages];
			for(j = 0; j < tmpPages; ++j)
				framenos[j] = ((uintptr_t)tbl + j * PAGE_SIZE) / PAGE_SIZE;
			tmp  = PageDir::mapToTemp(framenos,tmpPages);
			tmptbl = (RSDT*)(tmp + tbloff);
		}

		/* do we have to extend the mapping in the ACPI-area? */
		if(curDest + tmptbl->length > destEnd) {
			Log::printf("Skipping ACPI table %zu (doesn't fit anymore: %p vs. %p)\n",i,
					curDest + tmptbl->length,destEnd);
			PageDir::unmapFromTemp(tmpPages);
			continue;
		}

		if(checksumValid(tmptbl,tmptbl->length)) {
			/* copy the table */
			memcpy((void*)curDest,tmptbl,tmptbl->length);
			sll_append(&acpiTables,(void*)curDest);
			curDest += tmptbl->length;
		}
		else
			Log::printf("Checksum of table %zu is invalid. Skipping\n",i);
		PageDir::unmapFromTemp(tmpPages);
	}

	/* walk through the table and search for APIC entries */
	cpuid_t id = ::LAPIC::getId();
	sSLNode *n;
	for(n = sll_begin(&acpiTables); n != NULL; n = n->next) {
		RSDT *tbl = (RSDT*)n->data;
		if(checksumValid(tbl,tbl->length) && tbl->signature == SIG('A','P','I','C')) {
			RSDTAPIC *rapic = (RSDTAPIC*)tbl;
			APIC *apic = rapic->apics;
			while(apic < (APIC*)((uintptr_t)tbl + tbl->length)) {
				/* we're only interested in the local APICs */
				if(apic->type == TYPE_LAPIC) {
					LAPIC *lapic = (LAPIC*)apic;
					if(lapic->flags & 0x1)
						SMP::addCPU(lapic->id == id,lapic->id,false);
				}
				apic = (APIC*)((uintptr_t)apic + apic->length);
			}
		}
	}
}

bool ACPI::sigValid(const RSDP *r) {
	return r->signature[0] == SIG('R','S','D',' ') && r->signature[1] == SIG('P','T','R',' ');
}

bool ACPI::checksumValid(const void *r,size_t len) {
	uint8_t sum = 0;
	uint8_t *p;
	for(p = (uint8_t*)r; p < (uint8_t*)r + len; p++)
		sum += *p;
	return sum == 0;
}

ACPI::RSDP *ACPI::findIn(uintptr_t start,size_t len) {
	uintptr_t addr;
	for(addr = start; addr < start + len; addr += 16) {
		RSDP *r = (RSDP*)addr;
		if(sigValid(r) && checksumValid(r,20))
			return r;
	}
	return NULL;
}

void ACPI::print() {
	sSLNode *n;
	size_t i = 0;
	Video::printf("ACPI tables:\n");
	for(n = sll_begin(&acpiTables); n != NULL; n = n->next, i++) {
		RSDT *tbl = (RSDT*)n->data;
		Video::printf("\tTable%zu:\n",i);
		Video::printf("\t\tsignature: %.4s\n",(char*)&tbl->signature);
		Video::printf("\t\tlength: %u\n",tbl->length);
		Video::printf("\t\trevision: %u\n",tbl->revision);
		Video::printf("\t\toemId: %.6s\n",tbl->oemId);
		Video::printf("\t\toemTableId: %.8s\n",tbl->oemTableId);
		Video::printf("\n");
	}
}
