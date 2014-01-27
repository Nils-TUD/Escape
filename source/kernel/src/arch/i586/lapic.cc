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
#include <sys/arch/i586/lapic.h>
#include <sys/task/timer.h>
#include <sys/mem/pagedir.h>
#include <sys/cpu.h>
#include <sys/config.h>
#include <assert.h>

bool LAPIC::enabled;
uintptr_t LAPIC::apicAddr;

void LAPIC::init() {
	enabled = false;

	if(CPU::hasFeature(CPU::FEAT_APIC)) {
		/* TODO every APIC may have a different address */
		uint64_t apicBase = CPU::getMSR(MSR_APIC_BASE);
		if(apicBase & APIC_BASE_EN) {
			apicAddr = PageDir::makeAccessible(apicBase & 0xFFFFF000,1);
			enabled = true;
			CPU::setMSR(MSR_APIC_BASE,apicBase | 0x800);
			/* enable BSP LAPIC */
			enable();
		}
	}
}

void LAPIC::enableTimer() {
	write(REG_TIMER_ICR,(CPU::getBusSpeed() / TIMER_DIVIDER) / Timer::FREQUENCY_DIV);
	setLVT(REG_LVT_TIMER,Interrupts::IRQ_LAPIC,ICR_DELMODE_FIXED,UNMASKED,MODE_PERIODIC);
}

void LAPIC::writeIPI(uint32_t high,uint32_t low) {
	while((read(REG_ICR_LOW) & ICR_DELSTAT_PENDING))
		CPU::pause();
	write(REG_ICR_HIGH,high);
	write(REG_ICR_LOW,low);
}

void lapic_eoi() {
	LAPIC::eoi();
}
