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

#include <common.h>
#include <arch/x86/acpi.h>
#include <arch/x86/lapic.h>
#include <arch/x86/ioapic.h>
#include <task/smp.h>
#include <task/proc.h>
#include <vfs/dir.h>
#include <mem/pagedir.h>
#include <log.h>
#include <config.h>
#include <util.h>
#include <video.h>
#include <string.h>

#define MAX_ACPI_PAGES		32
#define BDA_EBDA			0x40E	/* TODO not always available? */
#define BIOS_AREA			0xE0000

bool ACPI::enabled = false;
ACPI::RSDP *ACPI::rsdp;
esc::ISList<sRSDT*> ACPI::acpiTables;

void ACPI::init() {
	if((enabled = ACPI::find()))
		ACPI::parse();
}

bool ACPI::find() {
	/* first kb of extended bios data area (EBDA) */
	uint16_t ebda = *(uint16_t*)(KERNEL_BEGIN | BDA_EBDA);
	if((rsdp = findIn(KERNEL_BEGIN | ebda * 16,1024)))
		return true;
	/* main bios area below 1 mb */
	if((rsdp = findIn(KERNEL_BEGIN | BIOS_AREA,0x20000)))
		return true;
	return false;
}

void ACPI::addTable(sRSDT *tbl,size_t i,uintptr_t &curDest,uintptr_t destEnd) {
	/* copy it page by page to our ACPI table area */
	size_t tbloff = (uintptr_t)tbl & (PAGE_SIZE - 1);
	frameno_t frm = (uintptr_t)tbl / PAGE_SIZE;
	sRSDT *tmptbl = (sRSDT*)(PageDir::getAccess(frm) + tbloff);
	size_t rem = tmptbl->length;

	if(curDest + rem > destEnd) {
		Log::get().writef("Skipping ACPI table %zu (doesn't fit anymore: %p vs. %p)\n",i,
				curDest + tmptbl->length,destEnd);
		PageDir::removeAccess(frm);
		return;
	}

	sRSDT *begin = (sRSDT*)curDest;
	do {
		size_t amount = MIN(rem,PAGE_SIZE - tbloff);
		memcpy((void*)curDest,tmptbl,amount);

		curDest += amount;
		rem -= amount;
		tbloff = 0;

		if(rem > 0) {
			PageDir::removeAccess(frm);
			tmptbl = (sRSDT*)PageDir::getAccess(++frm);
		}
	}
	while(rem > 0);
	PageDir::removeAccess(frm);

	if(acpi_checksumValid(begin,begin->length))
		acpiTables.append(begin);
	else
		Log::get().writef("Checksum of table %zu is invalid. Skipping\n",i);
}

void ACPI::parse() {
	Log::get().writef("Parsing ACPI tables...\n");

	sRSDT *rsdt = (sRSDT*)(uintptr_t)rsdp->rsdtAddr;
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
			tbl = (sRSDT*)(uintptr_t)*(uint32_t*)(start + i * size);
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
				addTable((sRSDT*)(uintptr_t)facp->DSDT,++count,curDest,destEnd);
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
							if(lapic->flags & 0x1)
								SMP::addCPU(lapic->id == id,lapic->id,false);
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

				/* make bootstrap CPU ready; we're currently running on it */
				if(SMP::getCPUCount() > 0)
					SMP::setId(id,0);

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
							/* TODO always use edge triggered. otherwise we might run into an endless
							 * irq loop because we have to go into userland to shut the irq down in
							 * some cases */
							/*if((intr->flags & INTI_TRIGGER_MASK) == INTI_TRIG_EDGE ||
									(intr->flags & INTI_TRIGGER_MASK) == INTI_TRIG_BUS)*/
								trig = IOAPIC::RED_TRIGGER_EDGE;
							/*else
								trig = IOAPIC::RED_TRIGGER_LEVEL;*/
							IOAPIC::configureIrq(intr->source,intr->gsi,IOAPIC::RED_DEL_FIXED,pol,trig);
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

void ACPI::createFiles() {
	/* create /sys/acpi */
	bool created;
	VFSNode *sys = NULL;
	if(VFSNode::request("/sys",NULL,&sys,&created,VFS_WRITE,0) != 0)
		return;
	VFSNode *acpidir = createObj<VFSDir>(KERNEL_PID,sys,(char*)"acpi",DIR_DEF_MODE);
	if(!acpidir)
		goto error;

	/* create files */
	for(auto it = acpiTables.cbegin(); it != acpiTables.cend(); ++it) {
		char *name = (char*)Cache::calloc(1,5);
		if(name) {
			strncpy(name,(char*)&(*it)->signature,4);
			VFSNode *node = createObj<VFSFile>(
				KERNEL_PID,acpidir,name,*it,static_cast<size_t>((*it)->length));
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
