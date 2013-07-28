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
#include <sys/arch/i586/mpconfig.h>
#include <sys/mem/paging.h>
#include <sys/task/smp.h>
#include <sys/util.h>

/* bios data area contains address of EBDA and the base memory size */
#define BDA_EBDA			0x40E	/* TODO not always available? */
#define BDA_MEMSIZE			0x413
#define BIOS_ROM_AREA		0xF0000

#define MPCTE_TYPE_PROC		0
#define MPCTE_LEN_PROC		20
#define MPCTE_TYPE_BUS		1
#define MPCTE_LEN_BUS		8
#define MPCTE_TYPE_IOAPIC	2
#define MPCTE_LEN_IOAPIC	8
#define MPCTE_TYPE_IOIRQ	3
#define MPCTE_LEN_IOIRQ		8
#define MPCTE_TYPE_LIRQ		4
#define MPCTE_LEN_LIRQ		8

#define MPF_SIGNATURE		0x5F504D5F	/* _MP_ */
#define MPC_SIGNATURE		0x504D4350	/* PCMP */

typedef struct {
	uint32_t signature;
	uint32_t mpConfigTable;
	uint8_t length;
	uint8_t specRev;
	uint8_t checkSum;
	uint8_t feature[5];
} A_PACKED sMPFloatPtr;

typedef struct {
	uint32_t signature;
	uint16_t baseTableLength;
	uint8_t specRev;
	uint8_t checkSum;
	char oemId[8];
	char productId[12];
	uint32_t oemTablePointer;
	uint16_t oemTableSize;
	uint16_t entryCount;
	uint32_t localAPICAddr;
	uint32_t reserved;
} A_PACKED sMPConfTableHeader;

typedef struct {
	uint8_t type;
	uint8_t localAPICId;
	uint8_t localAPICVersion;
	uint8_t cpuFlags;
	uint8_t cpuSignature[4];
	uint32_t featureFlags;
	uint32_t reserved[2];
} A_PACKED sMPConfProc;

typedef struct {
	uint8_t type;
	uint8_t ioAPICId;
	uint8_t ioAPICVersion;
	uint8_t enabled : 1,
			: 7;
	uint32_t baseAddr;
} A_PACKED sMPConfIOAPIC;

typedef struct {
	uint8_t type;
	uint8_t intrptType;
	uint16_t : 12,
		triggerMode : 2,
		polarity : 2;
	uint8_t srcBusId;
	uint8_t srcBusIRQ;
	uint8_t dstIOAPICId;
	uint8_t dstIOAPICInt;
} A_PACKED sMPConfIOIntrptEntry;

typedef struct {
	uint8_t type;
	uint8_t intrptType;
	uint16_t : 12,
		triggerMode : 2,
		polarity : 2;
	uint8_t srcBusId;
	uint8_t srcBusIRQ;
	uint8_t dstLAPICId;
	uint8_t dstLAPICInt;
} A_PACKED sMPConfLocalIntrptEntry;

static sMPFloatPtr *mpconf_search(void);
static sMPFloatPtr *mpconf_searchIn(uintptr_t start,size_t length);

static sMPFloatPtr *mpf;

bool mpconf_find(void) {
	mpf = mpconf_search();
	return mpf != NULL;
}

void mpconf_parse(void) {
	if(!mpf)
		return;

	size_t i;
	sMPConfProc *proc;
	sMPConfIOAPIC *ioapic;
	sMPConfIOIntrptEntry *ioint;
	uint8_t *ptr;
	sMPConfTableHeader *tbl = (sMPConfTableHeader*)(KERNEL_AREA | mpf->mpConfigTable);

	if(tbl->signature != MPC_SIGNATURE)
		util_panic("MP Config Table has invalid signature\n");

	ptr = (uint8_t*)tbl + sizeof(sMPConfTableHeader);
	for(i = 0; i < tbl->entryCount; i++) {
		switch(*ptr) {
			case MPCTE_TYPE_PROC:
				proc = (sMPConfProc*)ptr;
				/* cpu usable? */
				if(proc->cpuFlags & 0x1)
					SMP::addCPU((proc->cpuFlags & 0x2) ? true : false,proc->localAPICId,false);
				ptr += MPCTE_LEN_PROC;
				break;
			case MPCTE_TYPE_BUS:
				ptr += MPCTE_LEN_BUS;
				break;
			case MPCTE_TYPE_IOAPIC:
				ioapic = (sMPConfIOAPIC*)ptr;
				if(ioapic->enabled)
					IOAPIC::add(ioapic->ioAPICId,ioapic->ioAPICVersion,ioapic->baseAddr);
				ptr += MPCTE_LEN_IOAPIC;
				break;
			case MPCTE_TYPE_IOIRQ:
				ioint = (sMPConfIOIntrptEntry*)ptr;
				IOAPIC::setRedirection(ioint->dstIOAPICId,ioint->srcBusIRQ,ioint->dstIOAPICInt,
						ioint->intrptType,ioint->polarity,ioint->triggerMode);
				ptr += MPCTE_LEN_IOIRQ;
				break;
			case MPCTE_TYPE_LIRQ:
				ptr += MPCTE_LEN_LIRQ;
				break;
			default:
				util_panic("Unknown MP Config Table entry %x",*ptr);
				break;
		}
	}
}

static sMPFloatPtr *mpconf_search(void) {
	sMPFloatPtr *res = NULL;
	uint16_t memSize;
	/* first kb of extended bios data area (EBDA) */
	uint16_t ebda = *(uint16_t*)(KERNEL_AREA | BDA_EBDA);
	if((res = mpconf_searchIn(KERNEL_AREA | ebda * 16,1024)))
		return res;
	/* last kb of base memory */
	memSize = *(uint16_t*)(KERNEL_AREA | BDA_MEMSIZE);
	if((res = mpconf_searchIn(KERNEL_AREA | (memSize - 1) * 1024,1024)))
		return res;
	/* bios rom address space */
	if((res = mpconf_searchIn(KERNEL_AREA | BIOS_ROM_AREA,0x10000)))
		return res;
	return NULL;
}

static sMPFloatPtr *mpconf_searchIn(uintptr_t start,size_t length) {
	sMPFloatPtr *addr = (sMPFloatPtr*)start;
	uintptr_t end = start + length;
	while((uintptr_t)addr < end) {
		if(addr->signature == MPF_SIGNATURE) {
			size_t i;
			/* all bytes sum'ed up including the signature should yield 0 */
			uint8_t checksum = 0;
			uint8_t *bytes = (uint8_t*)addr;
			for(i = 0; i < sizeof(sMPFloatPtr); i++)
				checksum += bytes[i];
			if(checksum == 0)
				return addr;
		}
		addr++;
	}
	return NULL;
}
