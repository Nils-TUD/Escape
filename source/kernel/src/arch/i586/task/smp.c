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
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/ioapic.h>
#include <sys/arch/i586/acpi.h>
#include <sys/arch/i586/mpconfig.h>
#include <sys/task/smp.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <sys/util.h>
#include <string.h>

#define TRAMPOLINE_ADDR		0x7000

extern void apProtMode(void);

static klock_t smpLock;
static volatile size_t seenAPs = 0;
static cpuid_t *log2Phys;
static uint8_t trampoline[] = {
#if DEBUGGING
#	include "../../../../../build/i586-debug/kernel_tramp.dump"
#else
#	include "../../../../../build/i586-release/kernel_tramp.dump"
#endif
};

extern volatile uint waiting;
extern volatile uint waitlock;
extern volatile uint halting;
extern volatile uint flushed;

bool smp_init_arch(void) {
	apic_init();
	if(apic_isAvailable()) {
		bool enabled = false;
		/* try ACPI first to find the LAPICs */
		if(acpi_find()) {
			apic_enable();
			acpi_parse();
			enabled = true;
		}
		/* if that's not available, use MPConf */
		else if(mpconf_find()) {
			apic_enable();
			mpconf_parse();
			enabled = true;
		}

		if(enabled) {
			/* if we have not found CPUs by ACPI or MPConf, assume we have only the BSP */
			if(smp_getCPUCount() == 0) {
				enabled = false;
				return false;
			}
			/* from now on, we'll use the logical-id as far as possible; but remember the physical
			 * one for IPIs, e.g. */
			cpuid_t id = apic_getId();
			log2Phys = (cpuid_t*)cache_alloc(smp_getCPUCount() * sizeof(cpuid_t));
			log2Phys[0] = id;
			smp_setId(id,0);
			return true;
		}
	}
	return false;
}

cpuid_t smp_getPhysId(cpuid_t logId) {
	return log2Phys[logId];
}

void smp_pauseOthers(void) {
	sCPU **cpus = smp_getCPUs();
	size_t i,count,cpuCount = smp_getCPUCount();
	cpuid_t cur = smp_getCurId();
	waiting = 0;
	waitlock = 1;
	count = 0;
	for(i = 0; i < cpuCount; i++) {
		if(i != cur && cpus[i]->ready) {
			smp_sendIPI(i,IPI_WAIT);
			count++;
		}
	}
	/* wait until all CPUs are paused */
	while(waiting < count)
		__asm__ ("pause");
}

void smp_resumeOthers(void) {
	waitlock = 0;
}

void smp_haltOthers(void) {
	sCPU **cpus = smp_getCPUs();
	size_t i,count,cpuCount = smp_getCPUCount();
	cpuid_t cur = smp_getCurId();
	halting = 0;
	count = 0;
	for(i = 0; i < cpuCount; i++) {
		if(i != cur && cpus[i]->ready) {
			/* prevent that we want to halt/pause this one again */
			cpus[i]->ready = false;
			smp_sendIPI(i,IPI_HALT);
			count++;
		}
	}
	/* wait until all CPUs are halted */
	while(halting < count)
		__asm__ ("pause");
}

void smp_ensureTLBFlushed(void) {
	sCPU **cpus = smp_getCPUs();
	size_t i,count,cpuCount = smp_getCPUCount();;
	cpuid_t cur = smp_getCurId();
	flushed = 0;
	count = 0;
	for(i = 0; i < cpuCount; i++) {
		if(i != cur && cpus[i]->ready) {
			smp_sendIPI(i,IPI_FLUSH_TLB_ACK);
			count++;
		}
	}
	/* wait until all CPUs have flushed their TLB */
	while(halting == 0 && flushed < count)
		__asm__ ("pause");
}

void smp_sendIPI(cpuid_t id,uint8_t vector) {
	apic_sendIPITo(smp_getPhysId(id),vector);
}

void smp_apIsRunning(void) {
	cpuid_t phys,log;
	spinlock_aquire(&smpLock);
	phys = apic_getId();
	log = gdt_getCPUId();
	log2Phys[log] = phys;
	smp_setId(phys,log);
	smp_setReady(log);
	seenAPs++;
	spinlock_release(&smpLock);
}

void smp_start(void) {
	if(smp_isEnabled()) {
		size_t total,i;
		/* TODO thats not completely correct, according to the MP specification */
		/* we have to wait, check if apic is an 82489DX, ... */

		apic_sendInitIPI();

		/* simulate a delay of > 10ms; TODO we should use a timer for that */
		for(i = 0; i < 10000000; i++)
			;

		memcpy((void*)(TRAMPOLINE_ADDR | KERNEL_AREA),trampoline,ARRAY_SIZE(trampoline));
		/* give the trampoline the start-address */
		*(uint32_t*)((TRAMPOLINE_ADDR | KERNEL_AREA) + 2) = (uint32_t)&apProtMode;

		apic_sendStartupIPI(TRAMPOLINE_ADDR);

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
	smp_setReady(0);
}

cpuid_t smp_getCurId(void) {
	return gdt_getCPUId();
}
