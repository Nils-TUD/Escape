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
#include <sys/mem/paging.h>
#include <sys/task/smp.h>
#include <sys/util.h>
#include <sys/video.h>
#include <assert.h>

size_t IOAPIC::count;
IOAPIC::Instance IOAPIC::ioapics[MAX_IOAPICS];

void IOAPIC::add(uint8_t id,uint8_t version,uintptr_t addr) {
	frameno_t frame;
	if(count >= MAX_IOAPICS)
		Util::panic("Limit of I/O APICs (%d) reached",MAX_IOAPICS);

	ioapics[count].id = id;
	ioapics[count].version = version;
	ioapics[count].addr = (uint32_t*)PageDir::makeAccessible(addr,1);
	assert((addr & (PAGE_SIZE - 1)) == 0);
	count++;
}

void IOAPIC::setRedirection(uint8_t dstApic,uint8_t srcIRQ,uint8_t dstInt,uint8_t type,
		uint8_t polarity,uint8_t triggerMode) {
#if 0
	uint8_t vector = Interrupts::getVectorFor(srcIRQ);
	cpuid_t lapicId = SMP::getCurId();
	sIOAPIC *ioapic = get(dstApic);
	if(ioapic == NULL)
		Util::panic("Unable to find I/O APIC with id %#x\n",dstApic);
	write(ioapic,IOAPIC_REG_REDTBL + dstInt * 2 + 1,lapicId << 24);
	write(ioapic,IOAPIC_REG_REDTBL + dstInt * 2,
			RED_INT_MASK_DIS | RED_DESTMODE_PHYS | (polarity << 13) |
			(triggerMode << 15) | (type << 8) | vector);
#endif
}

void IOAPIC::print(OStream &os) {
	size_t i,j;
	os.writef("I/O APICs:\n");
	for(i = 0; i < count; i++) {
		os.writef("%d:\n",i);
		os.writef("\tid = %#x\n",ioapics[i].id);
		os.writef("\tversion = %#x\n",ioapics[i].version);
		os.writef("\tvirt. addr. = %#x\n",ioapics[i].addr);
		for(j = 0; j < RED_COUNT; j++) {
			os.writef("\tred[%d]: %#x:%#x\n",j,read(ioapics + i,IOAPIC_REG_REDTBL + j * 2),
					read(ioapics + i,IOAPIC_REG_REDTBL + j * 2 + 1));
		}
	}
}

IOAPIC::Instance *IOAPIC::get(uint8_t id) {
	size_t i;
	for(i = 0; i < count; i++) {
		if(ioapics[i].id == id)
			return ioapics + i;
	}
	return NULL;
}
