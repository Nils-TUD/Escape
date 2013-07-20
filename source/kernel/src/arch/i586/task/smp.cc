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
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/ioapic.h>
#include <sys/arch/i586/acpi.h>
#include <sys/arch/i586/mpconfig.h>
#include <sys/task/smp.h>
#include <sys/task/timer.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/video.h>
#include <sys/cpu.h>
#include <sys/spinlock.h>
#include <sys/util.h>
#include <string.h>

#define TRAMPOLINE_ADDR		0x7000

EXTERN_C void apProtMode();

cpuid_t *SMP::log2Phys;

static klock_t smpLock;
static volatile size_t seenAPs = 0;
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

bool SMPBase::init_arch() {
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
			if(getCPUCount() == 0) {
				enabled = false;
				return false;
			}
			/* from now on, we'll use the logical-id as far as possible; but remember the physical
			 * one for IPIs, e.g. */
			cpuid_t id = apic_getId();
			SMP::log2Phys = (cpuid_t*)Cache::alloc(getCPUCount() * sizeof(cpuid_t));
			SMP::log2Phys[0] = id;
			setId(id,0);
			return true;
		}
	}
	return false;
}

void SMPBase::pauseOthers() {
	const CPU **cpus = getCPUs();
	size_t i,count,cpuCount = getCPUCount();
	cpuid_t cur = getCurId();
	waiting = 0;
	waitlock = 1;
	count = 0;
	for(i = 0; i < cpuCount; i++) {
		if(i != cur && cpus[i]->ready) {
			sendIPI(i,IPI_WAIT);
			count++;
		}
	}
	/* wait until all CPUs are paused */
	while(waiting < count)
		__asm__ ("pause");
}

void SMPBase::resumeOthers() {
	waitlock = 0;
}

void SMPBase::haltOthers() {
	const CPU **cpus = getCPUs();
	size_t i,count,cpuCount = getCPUCount();
	cpuid_t cur = getCurId();
	halting = 0;
	count = 0;
	for(i = 0; i < cpuCount; i++) {
		if(i != cur && cpus[i]->ready) {
			/* prevent that we want to halt/pause this one again */
			getCPUById(i)->ready = false;
			sendIPI(i,IPI_HALT);
			count++;
		}
	}
	/* wait until all CPUs are halted */
	while(halting < count)
		__asm__ ("pause");
}

void SMPBase::ensureTLBFlushed() {
	const CPU **cpus = getCPUs();
	size_t i,count,cpuCount = getCPUCount();;
	cpuid_t cur = getCurId();
	flushed = 0;
	count = 0;
	for(i = 0; i < cpuCount; i++) {
		if(i != cur && cpus[i]->ready) {
			sendIPI(i,IPI_FLUSH_TLB_ACK);
			count++;
		}
	}
	/* wait until all CPUs have flushed their TLB */
	while(halting == 0 && flushed < count)
		__asm__ ("pause");
}

void SMP::apIsRunning() {
	cpuid_t phys,log;
	spinlock_aquire(&smpLock);
	phys = apic_getId();
	log = gdt_getCPUId();
	log2Phys[log] = phys;
	setId(phys,log);
	setReady(log);
	seenAPs++;
	spinlock_release(&smpLock);
}

void SMPBase::start() {
	if(isEnabled()) {
		size_t total;
		uint64_t start,end;
		/* TODO thats not completely correct, according to the MP specification */
		/* we have to check if apic is an 82489DX */

		memcpy((void*)(TRAMPOLINE_ADDR | KERNEL_AREA),trampoline,ARRAY_SIZE(trampoline));
		/* give the trampoline the start-address */
		*(uint32_t*)((TRAMPOLINE_ADDR | KERNEL_AREA) + 2) = (uint32_t)&apProtMode;

		apic_sendInitIPI();
		Timer::wait(10000);

		apic_sendStartupIPI(TRAMPOLINE_ADDR);
		Timer::wait(200);
		apic_sendStartupIPI(TRAMPOLINE_ADDR);
		Timer::wait(200);

		/* wait until all APs are running */
		total = getCPUCount() - 1;
		start = ::CPU::rdtsc();
		end = start + Timer::timeToCycles(2000000);
		while(::CPU::rdtsc() < end && seenAPs != total)
			__asm__ ("pause");
		if(seenAPs != total) {
			log_printf("Found %zu CPUs in 2s, expected %zu. Disabling SMP",seenAPs,total);
			disable();
		}
	}

	/* We needed the area 0x0 .. 0x00400000 because in the first phase the GDT was setup so that
	 * the stuff at 0xC0000000 has been mapped to 0x00000000. Therefore, after enabling paging
	 * (the GDT is still the same) we have to map 0x00000000 to our kernel-stuff so that we can
	 * still access the kernel (because segmentation translates 0xC0000000 to 0x00000000 before
	 * it passes it to the MMU).
	 * Now our GDT is setup in the "right" way, so that 0xC0000000 will arrive at the MMU.
	 * Therefore we can unmap the 0x0 area. */
	paging_gdtFinished();
	setReady(0);
}
