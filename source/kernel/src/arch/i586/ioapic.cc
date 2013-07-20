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

typedef struct {
	uint8_t id;
	uint8_t version;
	uint32_t *addr;
} sIOAPIC;

#define IOAPIC_RED_COUNT	24
#define MAX_IOAPICS			8
#define IOREGSEL			(0x0 / sizeof(uint32_t))
#define IOWIN				(0x10 / sizeof(uint32_t))

#define IOAPIC_REG_ID		0x0
#define IOAPIC_REG_VERSION	0x1
#define IOAPIC_REG_ARB		0x2
#define IOAPIC_REG_REDTBL	0x10

#define RED_DESTMODE_PHYS	(0 << 11)
#define RED_DESTMODE_LOG	(1 << 11)
#define RED_INT_MASK_DIS	(0 << 16)
#define RED_INT_MASK_EN		(1 << 16)

static sIOAPIC *ioapic_get(uint8_t id);
static uint32_t ioapic_read(sIOAPIC *ioapic,uint32_t reg);
static void ioapic_write(sIOAPIC *ioapic,uint32_t reg,uint32_t value);

static size_t count;
static sIOAPIC ioapics[MAX_IOAPICS];

void ioapic_add(uint8_t id,uint8_t version,uintptr_t addr) {
	frameno_t frame;
	if(count >= MAX_IOAPICS)
		util_panic("Limit of I/O APICs (%d) reached",MAX_IOAPICS);

	ioapics[count].id = id;
	ioapics[count].version = version;
	ioapics[count].addr = (uint32_t*)paging_makeAccessible(addr,1);
	assert((addr & (PAGE_SIZE - 1)) == 0);
	count++;
}

void ioapic_setRedirection(uint8_t dstApic,uint8_t srcIRQ,uint8_t dstInt,uint8_t type,
		uint8_t polarity,uint8_t triggerMode) {
#if 0
	uint8_t vector = intrpt_getVectorFor(srcIRQ);
	cpuid_t lapicId = SMP::getCurId();
	sIOAPIC *ioapic = ioapic_get(dstApic);
	if(ioapic == NULL)
		util_panic("Unable to find I/O APIC with id %#x\n",dstApic);
	ioapic_write(ioapic,IOAPIC_REG_REDTBL + dstInt * 2 + 1,lapicId << 24);
	ioapic_write(ioapic,IOAPIC_REG_REDTBL + dstInt * 2,
			RED_INT_MASK_DIS | RED_DESTMODE_PHYS | (polarity << 13) |
			(triggerMode << 15) | (type << 8) | vector);
#endif
}

void ioapic_print(void) {
	size_t i,j;
	vid_printf("I/O APICs:\n");
	for(i = 0; i < count; i++) {
		vid_printf("%d:\n",i);
		vid_printf("\tid = %#x\n",ioapics[i].id);
		vid_printf("\tversion = %#x\n",ioapics[i].version);
		vid_printf("\tvirt. addr. = %#x\n",ioapics[i].addr);
		for(j = 0; j < IOAPIC_RED_COUNT; j++) {
			vid_printf("\tred[%d]: %#x:%#x\n",j,ioapic_read(ioapics + i,IOAPIC_REG_REDTBL + j * 2),
					ioapic_read(ioapics + i,IOAPIC_REG_REDTBL + j * 2 + 1));
		}
	}
}

static sIOAPIC *ioapic_get(uint8_t id) {
	size_t i;
	for(i = 0; i < count; i++) {
		if(ioapics[i].id == id)
			return ioapics + i;
	}
	return NULL;
}

static uint32_t ioapic_read(sIOAPIC *ioapic,uint32_t reg) {
	ioapic->addr[IOREGSEL] = reg;
	return ioapic->addr[IOWIN];
}

static void ioapic_write(sIOAPIC *ioapic,uint32_t reg,uint32_t value) {
	ioapic->addr[IOREGSEL] = reg;
	ioapic->addr[IOWIN] = value;
}
