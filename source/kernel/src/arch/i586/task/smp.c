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
#include <sys/arch/i586/cpu.h>
#include <sys/arch/i586/apic.h>
#include <sys/task/smp.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/video.h>
#include <sys/klock.h>
#include <sys/util.h>
#include <string.h>

#define TRAMPOLINE_ADDR		0x7000

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

static void smp_parseConfTable(sMPConfTableHeader *tbl);
static sMPFloatPtr *smp_find(void);
static sMPFloatPtr *smp_findIn(uintptr_t start,size_t length);
extern void apProtMode(void);

static klock_t lock;
static volatile size_t seenAPs = 0;
static uint8_t trampoline[] = {
#if DEBUGGING
#	include "../../../../../build/i586-debug/kernel_tramp.dump"
#else
#	include "../../../../../build/i586-release/kernel_tramp.dump"
#endif
};

bool smp_init_arch(void) {
	apic_init();
	if(apic_isAvailable()) {
		sMPFloatPtr *mpf = smp_find();
		if(mpf) {
			smp_parseConfTable((sMPConfTableHeader*)(KERNEL_START | mpf->mpConfigTable));
			apic_enable();
			return true;
		}
	}
	return false;
}

void smp_apIsRunning(void) {
	klock_aquire(&lock);
	seenAPs++;
	klock_release(&lock);
}

void smp_start(void) {
	if(smp_isEnabled()) {
		size_t total,i;
		/* TODO thats not completely correct, according to the MP specification */
		/* we have to wait, check if apic is an 82489DX, ... */

		apic_sendInitIPI();

		/* simulate a delay of > 10ms; TODO we should use a timer for that */
		for(i = 0; i < 1000000; i++)
			;

		uintptr_t dest = TRAMPOLINE_ADDR;
		memcpy((void*)(dest | KERNEL_START),trampoline,ARRAY_SIZE(trampoline));
		/* give the trampoline the start-address */
		*(uint32_t*)((dest | KERNEL_START) + 2) = (uint32_t)&apProtMode;

		apic_sendStartupIPI(dest);

		/* wait until all APs are running */
		total = smp_getCPUCount() - 1;
		while(seenAPs != total)
			;
	}

	/* We needed the area 0x0 .. 0x00400000 because in the first phase the GDT was setup so that
	 * the stuff at 0xC0000000 has been mapped to 0x00000000. Therefore, after enabling paging
	 * (the GDT is still the same) we have to map 0x00000000 to our kernel-stuff so that we can
	 * still access the kernel (because segmentation translates 0xC0000000 to 0x00000000 before
	 * it passes it to the MMU).
	 * Now our GDT is setup in the "right" way, so that 0xC0000000 will arrive at the MMU.
	 * Therefore we can unmap the 0x0 area. */
	paging_gdtFinished();
}

cpuid_t smp_getCurId(void) {
	return apic_getId();
}

static void smp_parseConfTable(sMPConfTableHeader *tbl) {
	size_t i;
	sMPConfProc *proc;

	if(tbl->signature != MPC_SIGNATURE)
		util_panic("MP Config Table has invalid signature\n");

	uint8_t *ptr = (uint8_t*)tbl + sizeof(sMPConfTableHeader);
	for(i = 0; i < tbl->entryCount; i++) {
		switch(*ptr) {
			case MPCTE_TYPE_PROC:
				proc = (sMPConfProc*)ptr;
				/* cpu usable? */
				if(proc->cpuFlags & 0x1)
					smp_addCPU((proc->cpuFlags & 0x2) ? true : false,proc->localAPICId);
				ptr += MPCTE_LEN_PROC;
				break;
			case MPCTE_TYPE_BUS:
				ptr += MPCTE_LEN_BUS;
				break;
			case MPCTE_TYPE_IOAPIC:
				ptr += MPCTE_LEN_IOAPIC;
				break;
			case MPCTE_TYPE_IOIRQ:
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

static sMPFloatPtr *smp_find(void) {
	sMPFloatPtr *res = NULL;
	/* first kb of extended bios data area (EBDA) */
	uint16_t ebda = *(uint16_t*)(KERNEL_START | BDA_EBDA);
	if((res = smp_findIn(KERNEL_START | ebda * 16,1024)))
		return res;
	/* last kb of base memory */
	uint16_t memSize = *(uint16_t*)(KERNEL_START | BDA_MEMSIZE);
	if((res = smp_findIn(KERNEL_START | (memSize - 1) * 1024,1024)))
		return res;
	/* bios rom address space */
	if((res = smp_findIn(KERNEL_START | BIOS_ROM_AREA,0x10000)))
		return res;
	return NULL;
}

static sMPFloatPtr *smp_findIn(uintptr_t start,size_t length) {
	sMPFloatPtr *mpf = (sMPFloatPtr*)start;
	uintptr_t end = start + length;
	while((uintptr_t)mpf < end) {
		if(mpf->signature == MPF_SIGNATURE) {
			size_t i;
			/* all bytes sum'ed up including the signature should yield 0 */
			uint8_t checksum = 0;
			uint8_t *bytes = (uint8_t*)mpf;
			for(i = 0; i < sizeof(sMPFloatPtr); i++)
				checksum += bytes[i];
			if(checksum == 0)
				return mpf;
		}
		mpf++;
	}
	return NULL;
}
