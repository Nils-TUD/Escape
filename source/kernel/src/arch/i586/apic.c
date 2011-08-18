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
#include <sys/arch/i586/apic.h>
#include <sys/arch/i586/cpu.h>
#include <sys/mem/paging.h>
#include <assert.h>

#define MSR_APIC_BASE			0x1B
#define APIC_BASE_EN			(1 << 11)
#define APIC_BASE_ADDR(msr)		((msr) & 0xFFFFF000)

#define APIC_REG_APICID			0x20
#define APIC_REG_SPURINT		0xF0
#define APIC_REG_ICR_LOW		0x300
#define APIC_REG_ICR_HIGH		0x310
#define APIC_REG_TASK_PRIO		0x80

#define SPURINT_APIC_EN			(1 << 8)

#define ICR_DELMODE_FIXED		(0x0 << 8)
#define ICR_DELMODE_LOWPRIO		(0x1 << 8)
#define ICR_DELMODE_SMI			(0x2 << 8)
#define ICR_DELMODE_NMI			(0x4 << 8)
#define ICR_DELMODE_INIT		(0x5 << 8)
#define ICR_DELMODE_STARTUP		(0x6 << 8)

#define ICR_DEST_PHYS			(0x0 << 11)
#define ICR_DEST_LOG			(0x1 << 11)

#define ICR_DELSTAT_IDLE		(0x0 << 12)
#define ICR_DELSTAT_PENDING		(0x1 << 12)

#define ICR_LEVEL_DEASSERT		(0x0 << 14)
#define ICR_LEVEL_ASSERT		(0x1 << 14)

#define ICR_TRIGMODE_EDGE		(0x0 << 15)
#define ICR_TRIGMODE_LEVEL		(0x1 << 15)

#define ICR_DESTSHORT_NO		(0x0 << 18)
#define ICR_DESTSHORT_SELF		(0x1 << 18)
#define ICR_DESTSHORT_ALL		(0x2 << 18)
#define ICR_DESTSHORT_NOTSELF	(0x3 << 18)

static uint32_t apic_read(uint32_t reg);
static void apic_write(uint32_t reg,uint32_t value);

static bool enabled;

void apic_init(void) {
	enabled = false;

	if(cpu_hasLocalAPIC()) {
		/* TODO every APIC may have a different address */
		uint64_t apicBase = cpu_getMSR(MSR_APIC_BASE);
		if(apicBase & APIC_BASE_EN) {
			uintptr_t physAddr = APIC_BASE_ADDR(apicBase);
			frameno_t frame = physAddr / PAGE_SIZE;
			paging_map(APIC_AREA,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
			enabled = true;
		}
	}
}

cpuid_t apic_getId(void) {
	if(enabled)
		return apic_read(APIC_REG_APICID) >> 24;
	return 0;
}

bool apic_isAvailable(void) {
	return enabled;
}

void apic_enable(void) {
	/* accept all interrupts */
    apic_write(APIC_REG_TASK_PRIO,0);
	apic_write(APIC_REG_SPURINT,SPURINT_APIC_EN);
}

void apic_sendInitIPI(void) {
	apic_write(APIC_REG_ICR_HIGH,0);
	apic_write(APIC_REG_ICR_LOW,ICR_DESTSHORT_NOTSELF | ICR_TRIGMODE_EDGE | ICR_LEVEL_ASSERT |
			ICR_DELSTAT_IDLE | ICR_DEST_PHYS | ICR_DELMODE_INIT);
}

void apic_sendStartupIPI(uintptr_t startAddr) {
	apic_write(APIC_REG_ICR_HIGH,0);
	apic_write(APIC_REG_ICR_LOW,((startAddr / PAGE_SIZE) & 0xFF) |
			ICR_DESTSHORT_NOTSELF | ICR_TRIGMODE_EDGE | ICR_LEVEL_ASSERT | ICR_DELSTAT_IDLE |
			ICR_DEST_PHYS | ICR_DELMODE_STARTUP);
}

static uint32_t apic_read(uint32_t reg) {
	assert(enabled);
	return *(uint32_t*)(APIC_AREA + reg);
}

static void apic_write(uint32_t reg,uint32_t value) {
	assert(enabled);
	*(uint32_t*)(APIC_AREA + reg) = value;
}
