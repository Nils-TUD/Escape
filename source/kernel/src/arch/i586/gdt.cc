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
#include <sys/arch/i586/gdt.h>
#include <sys/mem/cache.h>
#include <sys/mem/physmem.h>
#include <sys/mem/paging.h>
#include <sys/task/smp.h>
#include <sys/task/proc.h>
#include <sys/video.h>
#include <sys/util.h>
#include <string.h>
/* for offsetof() */
#include <stddef.h>

/* whether to store the running thread in the gdt (for bochs) */
#define GDT_STORE_RUN_THREAD	1

/* == for the access-field == */

/* the GDT entry types */
#define GDT_TYPE_DATA			(0 | GDT_NOSYS)
#define GDT_TYPE_CODE			(1 << 3 | GDT_NOSYS)
#define GDT_TYPE_32BIT_TSS		(1 << 3 | 1 << 0)	/* requires GDT_NOSYS_SEG = 0 */

/* flag to switch between system- and code/data-segment */
#define GDT_NOSYS				(1 << 4)
#define GDT_PRESENT				(1 << 7)
#define GDT_DPL					(1 << 5 | 1 << 6)

#define GDT_DATA_WRITE			(1 << 1)
#define GDT_CODE_READ			(1 << 1)
#define GDT_CODE_CONFORMING		(1 << 2)

#define GDT_32BIT_PMODE			(1 << 6)
#define GDT_PAGE_GRANULARITY	(1 << 7)

/* privilege level */
#define GDT_DPL_KERNEL			0
#define GDT_DPL_USER			3

/* the offset of the io-bitmap */
#define IO_MAP_OFFSET			offsetof(TSS,ioMap)
/* an invalid offset for the io-bitmap => not loaded yet */
#define IO_MAP_OFFSET_INVALID	sizeof(TSS) + 16

GDT::Desc GDT::bspgdt[GDT_ENTRY_COUNT];
GDT::Table *GDT::allgdts;
size_t GDT::gdtCount = 1;
GDT::TSS GDT::bsptss A_ALIGNED(PAGE_SIZE);
GDT::TSS **GDT::alltss;
size_t GDT::tssCount = 1;
const uint8_t **GDT::ioMaps;
size_t GDT::cpuCount;
extern void *syscall_entry;

void GDT::init() {
	Table gdtTable;
	gdtTable.offset = (uintptr_t)bspgdt;
	gdtTable.size = GDT_ENTRY_COUNT * sizeof(Desc) - 1;

	/* clear gdt */
	memclear(bspgdt,GDT_ENTRY_COUNT * sizeof(Desc));

	/* kernel code */
	setDesc(bspgdt,1,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_CODE | GDT_PRESENT | GDT_CODE_READ,GDT_DPL_KERNEL);
	/* kernel data */
	setDesc(bspgdt,2,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,GDT_DPL_KERNEL);

	/* user code */
	setDesc(bspgdt,3,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_CODE | GDT_PRESENT | GDT_CODE_READ,GDT_DPL_USER);
	/* user data */
	setDesc(bspgdt,4,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,GDT_DPL_USER);
	/* tls */
	setDesc(bspgdt,5,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,GDT_DPL_USER);

	/* tss (leave a bit space for the vm86-segment-registers that will be present at the stack-top
	 * in vm86-mode. This way we can have the same interrupt-stack for all processes) */
	bsptss.esp0 = KERNEL_STACK_AREA + PAGE_SIZE - 2 * sizeof(int);
	bsptss.ss0 = 0x10;
	/* init io-map */
	bsptss.ioMapOffset = IO_MAP_OFFSET_INVALID;
	bsptss.ioMapEnd = 0xFF;
	setTSSDesc(bspgdt,6,(uintptr_t)&bsptss,sizeof(TSS) - 1);

	/* for the current thread */
	setDesc(bspgdt,7,0,0,GDT_TYPE_DATA | GDT_PRESENT,GDT_DPL_KERNEL);

	/* now load GDT and TSS */
	flush(&gdtTable);
	loadTSS(6 * sizeof(Desc));
}

void GDT::initBSP() {
	cpuCount = SMP::getCPUCount();
	allgdts = (Table*)Cache::calloc(cpuCount,sizeof(Table));
	if(!allgdts)
		Util::panic("Unable to allocate GDT-Tables for APs");
	alltss = (TSS**)Cache::calloc(cpuCount,sizeof(TSS*));
	if(!alltss)
		Util::panic("Unable to allocate TSS-pointers for APs");
	ioMaps = (const uint8_t**)Cache::calloc(cpuCount,sizeof(uint8_t*));
	if(!ioMaps)
		Util::panic("Unable to allocate IO-Map-Pointers for APs");

	/* put GDT of BSP into the GDT-array, for simplicity */
	allgdts[0].offset = (uintptr_t)bspgdt;
	allgdts[0].size = GDT_ENTRY_COUNT * sizeof(Desc) - 1;
	/* put TSS of BSP into the TSS-array */
	alltss[0] = &bsptss;

	CPU::setMSR(MSR_IA32_SYSENTER_CS,SEGSEL_GDTI_KCODE);
	CPU::setMSR(MSR_IA32_SYSENTER_ESP,bsptss.esp0);
	CPU::setMSR(MSR_IA32_SYSENTER_EIP,(uint64_t)&syscall_entry);
}

void GDT::initAP() {
	Table tmpTable;
	/* use the GDT of the BSP temporary. this way, we can access the heap and build our own gdt */
	tmpTable.offset = (uintptr_t)bspgdt;
	tmpTable.size = GDT_ENTRY_COUNT * sizeof(Desc) - 1;
	flush(&tmpTable);

	Table *gdttbl = allgdts + gdtCount++;
	size_t i = tssCount++;
	TSS *tss = (TSS*)PageDir::makeAccessible(0,BYTES_2_PAGES(sizeof(TSS)));

	/* create GDT (copy from first one) */
	Desc *apgdt = (Desc*)Cache::alloc(GDT_ENTRY_COUNT * sizeof(Desc));
	if(!apgdt)
		Util::panic("Unable to allocate GDT for AP");
	gdttbl->offset = (uintptr_t)apgdt;
	gdttbl->size = GDT_ENTRY_COUNT * sizeof(Desc) - 1;
	memcpy(apgdt,bspgdt,GDT_ENTRY_COUNT * sizeof(Desc));

	/* create TSS (copy from first one) */
	alltss[i] = tss;
	memcpy(tss,&bsptss,sizeof(TSS));
	tss->esp0 = KERNEL_STACK_AREA + PAGE_SIZE - 2 * sizeof(int);
	tss->ioMapOffset = IO_MAP_OFFSET_INVALID;
	tss->ioMapEnd = 0xFF;

	/* put tss in our gdt */
	setTSSDesc(apgdt,6,(uintptr_t)tss,sizeof(TSS) - 1);

	/* now load our GDT and TSS */
	flush(gdttbl);
	loadTSS(6 * sizeof(Desc));

	CPU::setMSR(MSR_IA32_SYSENTER_CS,SEGSEL_GDTI_KCODE);
	CPU::setMSR(MSR_IA32_SYSENTER_ESP,tss->esp0);
	CPU::setMSR(MSR_IA32_SYSENTER_EIP,(uint64_t)&syscall_entry);
}

cpuid_t GDT::getCPUId() {
	Table tbl;
	get(&tbl);
	for(size_t i = 0; i < cpuCount; i++) {
		if(allgdts[i].offset == tbl.offset)
			return i;
	}
	/* never reached */
	return -1;
}

void GDT::setRunning(cpuid_t id,Thread *t) {
	/* store the thread-pointer into an unused slot of the gdt */
	Desc *gdt = (Desc*)allgdts[id].offset;
	setDesc(gdt,7,(uintptr_t)t,0,GDT_TYPE_DATA | GDT_PRESENT,GDT_DPL_KERNEL);
}

cpuid_t GDT::prepareRun(Thread *old,Thread *n) {
	cpuid_t id = old == NULL ? getCPUId() : old->getCPU();
	/* the thread-control-block is at the end of the tls-region; %gs:0x0 should reference
	 * the thread-control-block; use 0xFFFFFFFF as limit because we want to be able to use
	 * %gs:0xFFFFFFF8 etc. */
	if(n->getTLSRegion()) {
		uintptr_t tlsEnd = n->getTLSRegion()->virt + n->getTLSRegion()->reg->getByteCount();
		setDesc((Desc*)allgdts[id].offset,5,tlsEnd - sizeof(void*),
				0xFFFFFFFF >> PAGE_SIZE_SHIFT,GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,
				GDT_DPL_USER);
	}
	alltss[id]->esp0 = n->getKernelStack() + PAGE_SIZE - 2 * sizeof(int);
	if(!old || old->getProc() != n->getProc())
		alltss[id]->ioMapOffset = IO_MAP_OFFSET_INVALID;
#if GDT_STORE_RUN_THREAD
	setRunning(id,n);
#endif
	CPU::setMSR(MSR_IA32_SYSENTER_ESP,alltss[id]->esp0);
	return id;
}

bool GDT::ioMapPresent() {
	Thread *t = Thread::getRunning();
	return alltss[t->getCPU()]->ioMapOffset != IO_MAP_OFFSET_INVALID;
}

void GDT::setIOMap(const uint8_t *ioMap,bool forceCpy) {
	Thread *t = Thread::getRunning();
	alltss[t->getCPU()]->ioMapOffset = IO_MAP_OFFSET;
	if(forceCpy || ioMap != ioMaps[t->getCPU()])
		memcpy(alltss[t->getCPU()]->ioMap,ioMap,IO_MAP_SIZE / 8);
	/* remove the map from other cpus; we have to copy it again because it has changed */
	if(forceCpy) {
		for(size_t i = 0; i < cpuCount; i++) {
			if(i != t->getCPU() && ioMaps[i] == ioMap)
				ioMaps[i] = NULL;
		}
	}
	ioMaps[t->getCPU()] = ioMap;
}

void GDT::setDesc(Desc *gdt,size_t index,uintptr_t address,size_t size,uint8_t access,uint8_t ringLevel) {
	gdt[index].addrLow = address & 0xFFFF;
	gdt[index].addrMiddle = (address >> 16) & 0xFF;
	gdt[index].addrHigh = (address >> 24) & 0xFF;
	gdt[index].sizeLow = size & 0xFFFF;
	gdt[index].sizeHigh = ((size >> 16) & 0xF) | GDT_PAGE_GRANULARITY | GDT_32BIT_PMODE;
	gdt[index].access = access | ((ringLevel & 3) << 5);
}

void GDT::setTSSDesc(Desc *gdt,size_t index,uintptr_t address,size_t size) {
	gdt[index].addrLow = address & 0xFFFF;
	gdt[index].addrMiddle = (address >> 16) & 0xFF;
	gdt[index].addrHigh = (address >> 24) & 0xFF;
	gdt[index].sizeLow = size & 0xFFFF;
	gdt[index].sizeHigh = ((size >> 16) & 0xF) | GDT_32BIT_PMODE;
	gdt[index].access = GDT_PRESENT | GDT_TYPE_32BIT_TSS | (GDT_DPL_KERNEL << 5);
}

void GDT::print(OStream &os) {
	os.writef("GDTs:\n");
	for(auto cpu = SMP::begin(); cpu != SMP::end(); ++cpu) {
		Desc *gdt = (Desc*)allgdts[cpu->id].offset;
		os.writef("\tGDT of CPU %d\n",cpu->id);
		if(gdt) {
			for(size_t i = 0; i < GDT_ENTRY_COUNT; i++) {
				os.writef("\t\t%d: address=%02x%02x:%04x, size=%02x%04x, access=%02x\n",
						i,gdt[i].addrHigh,gdt[i].addrMiddle,gdt[i].addrLow,
						gdt[i].sizeHigh,gdt[i].sizeLow,gdt[i].access);
			}
		}
		else
			os.writef("\t\t-\n");
	}
}
