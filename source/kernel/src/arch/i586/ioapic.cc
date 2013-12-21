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
#include <sys/arch/i586/ioapic.h>
#include <sys/mem/pagedir.h>
#include <sys/task/smp.h>
#include <sys/util.h>
#include <sys/log.h>
#include <sys/config.h>
#include <assert.h>

klock_t IOAPIC::lck;
size_t IOAPIC::count;
IOAPIC::Instance IOAPIC::ioapics[MAX_IOAPICS];
uint IOAPIC::isa2gsi[ISA_IRQ_COUNT];

void IOAPIC::add(uint8_t id,uintptr_t addr,uint baseGSI) {
	if(Config::get(Config::FORCE_PIC))
		return;
	if(count >= MAX_IOAPICS)
		Util::panic("Limit of I/O APICs (%d) reached",MAX_IOAPICS);

	/* lazy init isa2gsi to identity mapping */
	if(count == 0) {
		for(size_t i = 0; i < ISA_IRQ_COUNT; ++i)
			isa2gsi[i] = i;
	}

	Instance *inst = ioapics + count;
	inst->id = id;
	inst->addr = (uint32_t*)PageDir::makeAccessible(addr,1);
	inst->baseGSI = baseGSI;
	inst->version = getVersion(ioapics + count);
	inst->count = getMax(ioapics + count) + 1;
	Log::get().writef("IOAPIC: id=%#x version=%#x addr=%p gsis=%u..%u\n",
		inst->id,inst->version,inst->addr,inst->baseGSI,inst->baseGSI + inst->count - 1);
	assert((addr & (PAGE_SIZE - 1)) == 0);
	count++;
}

void IOAPIC::setRedirection(uint8_t srcIRQ,uint gsi,DeliveryMode delivery,
		Polarity polarity,TriggerMode triggerMode) {
	if(Config::get(Config::FORCE_PIC))
		return;

	int vector = Interrupts::getVectorFor(srcIRQ);
	if(vector == -1 || exists(vector))
		return;

	assert(srcIRQ < ISA_IRQ_COUNT);
	isa2gsi[srcIRQ] = gsi;

	cpuid_t lapicId = LAPIC::getId();
	IOAPIC::Instance *ioapic = get(gsi);
	if(ioapic != NULL) {
		gsi -= ioapic->baseGSI;
		write(ioapic,IOAPIC_REG_REDTBL + gsi * 2 + 1,lapicId << 24);
		write(ioapic,IOAPIC_REG_REDTBL + gsi * 2,
			RED_INT_MASK_DIS | RED_DESTMODE_PHYS | polarity | triggerMode | delivery | vector);

		Log::get().writef("INTSO: irq=%u gsi=%u del=%d pol=%d trig=%d\n",
			srcIRQ,gsi,(delivery >> 8) & 7,(polarity >> 13) & 1,(triggerMode >> 15) & 1);
	}
}

bool IOAPIC::exists(uint vector) {
	for(size_t i = 0; i < count; i++) {
		for(uint gsi = ioapics[i].baseGSI; gsi < ioapics[i].baseGSI + ioapics[i].count; ++gsi) {
			uint32_t lower = read(ioapics + i,IOAPIC_REG_REDTBL + gsi * 2);
			if(!(lower & 0x10000) && (lower & 0xFF) == vector)
				return true;
		}
	}
	return false;
}

IOAPIC::Instance *IOAPIC::get(uint gsi) {
	for(size_t i = 0; i < count; i++) {
		if(gsi >= ioapics[i].baseGSI && gsi < ioapics[i].baseGSI + ioapics[i].count)
			return ioapics + i;
	}
	return NULL;
}

void IOAPIC::print(OStream &os) {
	static const char *pols[] = {"High","Low "};
	static const char *triggers[] = {"Edge ","Level"};
	static const char *delivery[] = {"Fix","Low","SMI","???","NMI","INI","???","Ext"};
	os.writef("I/O APICs:\n");
	for(size_t i = 0; i < count; i++) {
		os.writef("%d:\n",i);
		os.writef("\tid = %#x\n",ioapics[i].id);
		os.writef("\tversion = %#x\n",ioapics[i].version);
		os.writef("\taddress = %p\n",ioapics[i].addr);
		for(size_t j = 0; j < RED_COUNT; j++) {
			uint32_t lower = read(ioapics + i,IOAPIC_REG_REDTBL + j * 2);
			uint32_t upper = read(ioapics + i,IOAPIC_REG_REDTBL + j * 2 + 1);
			os.writef("\t[%2d]: LAPIC=%d, pol=%s, trig=%s, del=%s vec=%2d, en=%d\n",
				j,upper >> 24,pols[(lower >> 13) & 0x1],triggers[(lower >> 15) & 1],
				delivery[(lower >> 8) & 0x7],lower & 0xFF,!(lower & 0x10000));
		}
	}
}
