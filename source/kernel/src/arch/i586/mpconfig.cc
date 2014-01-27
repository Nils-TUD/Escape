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

#include <sys/common.h>
#include <sys/arch/i586/ioapic.h>
#include <sys/arch/i586/mpconfig.h>
#include <sys/mem/pagedir.h>
#include <sys/task/smp.h>
#include <sys/log.h>
#include <sys/util.h>

/* bios data area contains address of EBDA and the base memory size */
#define BDA_EBDA			0x40E	/* TODO not always available? */
#define BDA_MEMSIZE			0x413
#define BIOS_ROM_AREA		0xF0000

#define MPF_SIGNATURE		0x5F504D5F	/* _MP_ */
#define MPC_SIGNATURE		0x504D4350	/* PCMP */

MPConfig::FloatPtr *MPConfig::mpf;

bool MPConfig::find() {
	mpf = search();
	return mpf != NULL;
}

void MPConfig::handleIOInt(IOIntrptEntry *ioint) {
	IOAPIC::Polarity pol;
	IOAPIC::TriggerMode trig;
	IOAPIC::DeliveryMode del;
	/* assume high active if it conforms to the bus specification */
	/* TODO consider other buses */
	if(ioint->polarity == POL_HIGH_ACTIVE || ioint->polarity == POL_BUS)
		pol = IOAPIC::RED_POL_HIGH_ACTIVE;
	else
		pol = IOAPIC::RED_POL_LOW_ACTIVE;
	/* assume edge-triggered if it conforms to the bus specification */
	if(ioint->triggerMode == TRIG_EDGE || ioint->triggerMode == TRIG_BUS)
		trig = IOAPIC::RED_TRIGGER_EDGE;
	else
		trig = IOAPIC::RED_TRIGGER_LEVEL;
	switch(ioint->intrptType) {
		default:
		case TYPE_INT:
			del = IOAPIC::RED_DEL_FIXED;
			break;
		case TYPE_NMI:
			del = IOAPIC::RED_DEL_NMI;
			break;
		case TYPE_SMI:
			del = IOAPIC::RED_DEL_SMI;
			break;
		case TYPE_EXTINT:
			del = IOAPIC::RED_DEL_EXTINT;
			break;
	}
	IOAPIC::setRedirection(ioint->srcBusIRQ,ioint->dstIOAPICInt,del,pol,trig);
}

void MPConfig::parse() {
	if(!mpf)
		return;

	Proc *proc;
	MPIOAPIC *ioapic;
	IOIntrptEntry *ioint;
	TableHeader *tbl = (TableHeader*)(KERNEL_AREA | mpf->mpConfigTable);

	if(tbl->signature != MPC_SIGNATURE)
		Util::panic("MP Config Table has invalid signature\n");

	Log::get().writef("Parsing MP configuration table...\n");
	uint8_t *ptr = (uint8_t*)tbl + sizeof(TableHeader);
	for(size_t i = 0; i < tbl->entryCount; i++) {
		switch(*ptr) {
			case MPCTE_TYPE_PROC:
				proc = (Proc*)ptr;
				/* cpu usable? */
				if(proc->cpuFlags & 0x1)
					SMP::addCPU((proc->cpuFlags & 0x2) ? true : false,proc->localAPICId,false);
				ptr += MPCTE_LEN_PROC;
				break;
			case MPCTE_TYPE_BUS:
				ptr += MPCTE_LEN_BUS;
				break;
			case MPCTE_TYPE_IOAPIC:
				ioapic = (MPIOAPIC*)ptr;
				if(ioapic->enabled)
					IOAPIC::add(ioapic->ioAPICId,ioapic->baseAddr,0);
				ptr += MPCTE_LEN_IOAPIC;
				break;
			case MPCTE_TYPE_IOIRQ:
				ioint = (IOIntrptEntry*)ptr;
				handleIOInt(ioint);
				ptr += MPCTE_LEN_IOIRQ;
				break;
			case MPCTE_TYPE_LIRQ:
				ptr += MPCTE_LEN_LIRQ;
				break;
			default:
				Util::panic("Unknown MP Config Table entry %x",*ptr);
				break;
		}
	}
}

MPConfig::FloatPtr *MPConfig::search() {
	FloatPtr *res = NULL;
	/* first kb of extended bios data area (EBDA) */
	uint16_t ebda = *(uint16_t*)(KERNEL_AREA | BDA_EBDA);
	if((res = searchIn(KERNEL_AREA | ebda * 16,1024)))
		return res;
	/* last kb of base memory */
	uint16_t memSize = *(uint16_t*)(KERNEL_AREA | BDA_MEMSIZE);
	if((res = searchIn(KERNEL_AREA | (memSize - 1) * 1024,1024)))
		return res;
	/* bios rom address space */
	if((res = searchIn(KERNEL_AREA | BIOS_ROM_AREA,0x10000)))
		return res;
	return NULL;
}

MPConfig::FloatPtr *MPConfig::searchIn(uintptr_t start,size_t length) {
	FloatPtr *addr = (FloatPtr*)start;
	uintptr_t end = start + length;
	while((uintptr_t)addr < end) {
		if(addr->signature == MPF_SIGNATURE) {
			/* all bytes sum'ed up including the signature should yield 0 */
			uint8_t checksum = 0;
			uint8_t *bytes = (uint8_t*)addr;
			for(size_t i = 0; i < sizeof(FloatPtr); i++)
				checksum += bytes[i];
			if(checksum == 0)
				return addr;
		}
		addr++;
	}
	return NULL;
}
