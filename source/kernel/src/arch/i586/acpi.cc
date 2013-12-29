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
#include <sys/arch/i586/ioapic.h>
#include <sys/task/smp.h>
#include <sys/task/proc.h>
#include <sys/vfs/dir.h>
#include <sys/mem/pagedir.h>
#include <sys/log.h>
#include <sys/config.h>
#include <sys/util.h>
#include <sys/video.h>
#include <string.h>

#define MAX_ACPI_PAGES		32
#define BDA_EBDA			0x40E	/* TODO not always available? */
#define BIOS_AREA			0xE0000

bool ACPI::enabled = false;
ACPI::RSDP *ACPI::rsdp;
ISList<sRSDT*> ACPI::acpiTables;

void ACPI::init() {
	if((enabled = ACPI::find()))
		ACPI::parse();
}

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

void ACPI::addTable(sRSDT *tbl,size_t i,uintptr_t &curDest,uintptr_t destEnd) {
	/* map it to a temporary place; try one page first and extend it if necessary */
	size_t tmpPages = 1;
	size_t tbloff = (uintptr_t)tbl & (PAGE_SIZE - 1);
	frameno_t frm = (uintptr_t)tbl / PAGE_SIZE;
	uintptr_t tmp = PageDir::mapToTemp(&frm,1);
	sRSDT *tmptbl = (sRSDT*)(tmp + tbloff);
	if((uintptr_t)tmptbl + tmptbl->length > tmp + PAGE_SIZE) {
		size_t tbllen = tmptbl->length;
		PageDir::unmapFromTemp(1);
		/* determine the real number of required pages */
		tmpPages = (tbloff + tbllen + PAGE_SIZE - 1) / PAGE_SIZE;
		if(tmpPages > TEMP_MAP_AREA_SIZE / PAGE_SIZE) {
			Log::get().writef("Skipping ACPI table %zu (too large: %zu)\n",i,tbllen);
			return;
		}
		/* map it again */
		frameno_t framenos[tmpPages];
		for(size_t j = 0; j < tmpPages; ++j)
			framenos[j] = ((uintptr_t)tbl + j * PAGE_SIZE) / PAGE_SIZE;
		tmp  = PageDir::mapToTemp(framenos,tmpPages);
		tmptbl = (sRSDT*)(tmp + tbloff);
	}

	/* do we have to extend the mapping in the ACPI-area? */
	if(curDest + tmptbl->length > destEnd) {
		Log::get().writef("Skipping ACPI table %zu (doesn't fit anymore: %p vs. %p)\n",i,
				curDest + tmptbl->length,destEnd);
		PageDir::unmapFromTemp(tmpPages);
		return;
	}

	if(acpi_checksumValid(tmptbl,tmptbl->length)) {
		/* copy the table */
		memcpy((void*)curDest,tmptbl,tmptbl->length);
		acpiTables.append((sRSDT*)curDest);
		curDest += tmptbl->length;
	}
	else
		Log::get().writef("Checksum of table %zu is invalid. Skipping\n",i);
	PageDir::unmapFromTemp(tmpPages);
}

void ACPI::parse() {
	Log::get().writef("Parsing ACPI tables...\n");

	sRSDT *rsdt = (sRSDT*)rsdp->rsdtAddr;
	size_t size = sizeof(uint32_t);
	/* check for extended system descriptor table */
	if(rsdp->revision > 1 && acpi_checksumValid(rsdp,rsdp->length)) {
		rsdt = (sRSDT*)rsdp->xsdtAddr;
		size = sizeof(uint64_t);
	}

	/* first map the sRSDT. we assume that it covers only one page */
	size_t off = (uintptr_t)rsdt & (PAGE_SIZE - 1);
	rsdt = (sRSDT*)(PageDir::makeAccessible((uintptr_t)rsdt,1) + off);
	size_t count = (rsdt->length - sizeof(sRSDT)) / size;
	if(off + rsdt->length > PAGE_SIZE) {
		size_t pages = BYTES_2_PAGES(off + rsdt->length);
		rsdt = (sRSDT*)(PageDir::makeAccessible((uintptr_t)rsdt,pages) + off);
	}

	/* now walk through the tables behind the sRSDT */
	uintptr_t curDest = PageDir::makeAccessible(0,MAX_ACPI_PAGES);
	uintptr_t destEnd = curDest + MAX_ACPI_PAGES * PAGE_SIZE;
	uintptr_t start = (uintptr_t)(rsdt + 1);
	for(size_t i = 0; i < count; i++) {
		sRSDT *tbl;
		if(size == sizeof(uint32_t))
			tbl = (sRSDT*)*(uint32_t*)(start + i * size);
		else
			tbl = (sRSDT*)*(uint64_t*)(start + i * size);
		addTable(tbl,i,curDest,destEnd);
	}

	/* walk through the table and search for APIC entries */
	cpuid_t id = ::LAPIC::getId();
	for(auto it = acpiTables.cbegin(); it != acpiTables.cend(); ++it) {
		switch((*it)->signature) {
			case ACPI_SIG('F','A','C','P'): {
				RSDTFACP *facp = (RSDTFACP*)(*it);
				addTable((sRSDT*)facp->DSDT,++count,curDest,destEnd);
				break;
			}

			case ACPI_SIG('A','P','I','C'): {
				RSDTAPIC *rapic = (RSDTAPIC*)(*it);
				/* collect the IOAPICs and LAPICs first */
				APIC *apic = rapic->apics;
				while(apic < (APIC*)((uintptr_t)(*it) + (*it)->length)) {
					switch(apic->type) {
						case TYPE_LAPIC: {
							APICLAPIC *lapic = (APICLAPIC*)apic;
							if(lapic->flags & 0x1) {
								SMP::addCPU(lapic->id == id,lapic->id,false);
								/* make bootstrap CPU ready; we're currently running on it */
								if(lapic->id == id)
									SMP::setId(id,0);
							}
						}
						break;

						case TYPE_IOAPIC: {
							APICIOAPIC *ioapic = (APICIOAPIC*)apic;
							IOAPIC::add(ioapic->id,ioapic->address,ioapic->baseGSI);
						}
						break;
					}
					apic = (APIC*)((uintptr_t)apic + apic->length);
				}

				/* now set the redirections */
				apic = rapic->apics;
				while(apic < (APIC*)((uintptr_t)(*it) + (*it)->length)) {
					switch(apic->type) {
						case TYPE_INTR: {
							APICIntSO *intr = (APICIntSO*)apic;
							IOAPIC::Polarity pol;
							IOAPIC::TriggerMode trig;
							/* assume high active if it conforms to the bus specification */
							/* TODO consider other buses */
							if((intr->flags & INTI_POL_MASK) == INTI_POL_HIGH_ACTIVE ||
								(intr->flags & INTI_POL_MASK) == INTI_POL_BUS)
								pol = IOAPIC::RED_POL_HIGH_ACTIVE;
							else
								pol = IOAPIC::RED_POL_LOW_ACTIVE;
							/* assume edge-triggered if it conforms to the bus specification */
							if((intr->flags & INTI_TRIGGER_MASK) == INTI_TRIG_EDGE ||
									(intr->flags & INTI_TRIGGER_MASK) == INTI_TRIG_BUS)
								trig = IOAPIC::RED_TRIGGER_EDGE;
							else
								trig = IOAPIC::RED_TRIGGER_LEVEL;
							IOAPIC::setRedirection(intr->source,intr->gsi,IOAPIC::RED_DEL_FIXED,pol,trig);
						}
						break;
					}
					apic = (APIC*)((uintptr_t)apic + apic->length);
				}
				break;
			}
		}
	}
}

void ACPI::create_files() {
	/* create /system/acpi */
	bool created;
	VFSNode *sys = NULL;
	if(VFSNode::request("/system",&sys,&created,VFS_WRITE,0) != 0)
		return;
	VFSNode *acpidir = CREATE(VFSDir,KERNEL_PID,sys,(char*)"acpi",DIR_DEF_MODE);
	if(!acpidir)
		goto error;

	/* create files */
	for(auto it = acpiTables.cbegin(); it != acpiTables.cend(); ++it) {
		char *name = (char*)Cache::calloc(1,5);
		if(name) {
			strncpy(name,(char*)&(*it)->signature,4);
			VFSNode *node = CREATE(VFSFile,KERNEL_PID,acpidir,name,*it,(*it)->length);
			if(!node) {
				Log::get().writef("Unable to create ACPI table %s\n",name);
				Cache::free(name);
			}
			else
				VFSNode::release(node);
		}
	}

	VFSNode::release(acpidir);
error:
	VFSNode::release(sys);
}

bool ACPI::sigValid(const RSDP *r) {
	return r->signature[0] == ACPI_SIG('R','S','D',' ') && r->signature[1] == ACPI_SIG('P','T','R',' ');
}

ACPI::RSDP *ACPI::findIn(uintptr_t start,size_t len) {
	for(uintptr_t addr = start; addr < start + len; addr += 16) {
		RSDP *r = (RSDP*)addr;
		if(sigValid(r) && acpi_checksumValid(r,20))
			return r;
	}
	return NULL;
}

void ACPI::print(OStream &os) {
	size_t i = 0;
	os.writef("ACPI tables:\n");
	for(auto it = acpiTables.cbegin(); it != acpiTables.cend(); ++it, ++i) {
		os.writef("\tTable%zu:\n",i);
		os.writef("\t\tsignature: %.4s\n",(char*)&(*it)->signature);
		os.writef("\t\tlength: %u\n",(*it)->length);
		os.writef("\t\trevision: %u\n",(*it)->revision);
		os.writef("\t\toemId: %.6s\n",(*it)->oemId);
		os.writef("\t\toemTableId: %.8s\n",(*it)->oemTableId);
		os.writef("\n");
	}
}
