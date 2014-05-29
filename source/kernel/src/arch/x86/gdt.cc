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
#include <sys/arch/x86/gdt.h>
#include <sys/mem/cache.h>
#include <sys/mem/physmem.h>
#include <sys/mem/pagedir.h>
#include <sys/task/smp.h>
#include <sys/task/proc.h>
#include <sys/syscalls.h>
#include <sys/video.h>
#include <sys/util.h>
#include <string.h>
/* for offsetof() */
#include <stddef.h>

/* whether to store the running thread in the gdt (for bochs) */
#define GDT_STORE_RUN_THREAD	0

static_assert(TSS::IO_MAP_OFFSET == offsetof(TSS,ioMap),"Wrong ioMap offset");

GDT::Desc GDT::bspgdt[GDT_ENTRY_COUNT];
TSS GDT::bspTSS A_ALIGNED(PAGE_SIZE);
GDT::PerCPU GDT::bsp;

GDT::PerCPU *GDT::all;
size_t GDT::cpuCount;

extern void *syscall_entry;

void GDT::init() {
	Table gdtTable;
	gdtTable.offset = (ulong)bspgdt;
	gdtTable.size = GDT_ENTRY_COUNT * sizeof(Desc) - 1;

	/* kernel code+data */
	setDesc(bspgdt + SEG_KCODE,0,~0UL >> PAGE_BITS,GRANU_PAGES,CODE_XR,DPL_KERNEL);
	setDesc(bspgdt + SEG_KDATA,0,~0UL >> PAGE_BITS,GRANU_PAGES,DATA_RW,DPL_KERNEL);
	/* user code+data */
	setDesc(bspgdt + SEG_UCODE,0,~0UL >> PAGE_BITS,GRANU_PAGES,CODE_XR,DPL_USER);
	setDesc(bspgdt + SEG_UDATA,0,~0UL >> PAGE_BITS,GRANU_PAGES,DATA_RW,DPL_USER);
	/* we use a second code-segment because sysret expects code to be IA32_STAR[63:48] + 16 and
	 * stack to be IA32_STAR[63:48] + 8. */
	setDesc(bspgdt + SEG_UCODE2,0,~0UL >> PAGE_BITS,GRANU_PAGES,CODE_XR,DPL_USER);
	/* tls */
	setDesc(bspgdt + SEG_TLS,0,~0UL >> PAGE_BITS,GRANU_PAGES,DATA_RW,DPL_USER);
	/* for the current thread */
	setDesc(bspgdt + SEG_THREAD,0,0,GRANU_PAGES,DATA_RW,DPL_KERNEL);
	/* tss */
	setTSS(bspgdt,&bspTSS,KSTACK_AREA);

	/* now load GDT and TSS */
	flush(&gdtTable);
	loadTSS(SEG_TSS * sizeof(Desc));
}

void GDT::initBSP() {
	all = (PerCPU*)Cache::calloc(SMP::getCPUCount(),sizeof(PerCPU));
	if(!all)
		Util::panic("Unable to allocate GDT's PerCPU array");

	all[0].gdt.offset = (uintptr_t)bspgdt;
	all[0].gdt.size = GDT_ENTRY_COUNT * sizeof(Desc) - 1;
	all[0].tss = &bspTSS;
	all[0].lastMSR = 0;

	setupSyscalls(&bspTSS);
	cpuCount++;
}

void GDT::initAP() {
	Table tmpTable;
	/* use the GDT of the BSP temporary. this way, we can access the heap and build our own gdt */
	tmpTable.offset = (uintptr_t)bspgdt;
	tmpTable.size = GDT_ENTRY_COUNT * sizeof(Desc) - 1;
	flush(&tmpTable);

	TSS *tss = (TSS*)PageDir::makeAccessible(0,BYTES_2_PAGES(sizeof(TSS)));

	/* create GDT (copy from first one) */
	Desc *apgdt = (Desc*)Cache::alloc(GDT_ENTRY_COUNT * sizeof(Desc));
	if(!apgdt)
		Util::panic("Unable to allocate GDT for AP");
	all[cpuCount].gdt.offset = (uintptr_t)apgdt;
	all[cpuCount].gdt.size = GDT_ENTRY_COUNT * sizeof(Desc) - 1;
	memcpy(apgdt,bspgdt,GDT_ENTRY_COUNT * sizeof(Desc));

	/* create TSS (copy from first one) */
	all[cpuCount].tss = tss;
	memcpy(tss,&bspTSS,sizeof(TSS));
	setTSS(apgdt,tss,KSTACK_AREA);

	/* now load our GDT and TSS */
	flush(&all[cpuCount].gdt);
	loadTSS(SEG_TSS * sizeof(Desc));

	setupSyscalls(tss);
	cpuCount++;
}

cpuid_t GDT::getCPUId() {
	Table tbl;
	get(&tbl);
	for(size_t i = 0; i < cpuCount; i++) {
		if(all[i].gdt.offset == tbl.offset)
			return i;
	}
	/* should not be reached. but if we do, assume that the id is 0 (-> no SMP) */
	return 0;
}

void GDT::setRunning(cpuid_t id,Thread *t) {
	/* store the thread-pointer into an unused slot of the gdt */
	Desc *gdt = (Desc*)all[id].gdt.offset;
	setDesc(gdt + SEG_THREAD,(uintptr_t)t,0,GRANU_PAGES,DATA_RO,DPL_KERNEL);
}

void GDT::prepareRun(cpuid_t id,bool newProc,Thread *n) {
	/* the thread-control-block is at the end of the tls-region; %gs:0x0 should reference
	 * the thread-control-block; use 0xFFFFFFFF as limit because we want to be able to use
	 * %gs:0xFFFFFFF8 etc. */
	if(EXPECT_FALSE(n->getTLSRegion())) {
		uintptr_t tlsEnd = n->getTLSRegion()->virt() + n->getTLSRegion()->reg->getByteCount();
		setDesc((Desc*)all[id].gdt.offset + SEG_TLS,tlsEnd - sizeof(void*),
				0xFFFFFFFF >> PAGE_BITS,GRANU_PAGES,DATA_RW,DPL_USER);
	}

	if(EXPECT_TRUE(newProc))
		all[id].tss->ioMapOffset = TSS::IO_MAP_OFFSET_INVALID;

#if GDT_STORE_RUN_THREAD
	setRunning(id,n);
#endif

	all[id].tss->setSP(n->getKernelStack() + PAGE_SIZE - 2 * sizeof(ulong));
#if defined(__i586__)
	if(EXPECT_FALSE(all[id].lastMSR != all[id].tss->REG(sp0))) {
		CPU::setMSR(CPU::MSR_IA32_SYSENTER_ESP,all[id].tss->REG(sp0));
		all[id].lastMSR = all[id].tss->REG(sp0);
	}
#else
	extern ulong kstackPtr;
	kstackPtr = n->getKernelStack() + PAGE_SIZE - sizeof(ulong);
#endif
}

bool GDT::ioMapPresent() {
	Thread *t = Thread::getRunning();
	return all[t->getCPU()].tss->ioMapOffset != TSS::IO_MAP_OFFSET_INVALID;
}

void GDT::setIOMap(const uint8_t *ioMap,bool forceCpy) {
	Thread *t = Thread::getRunning();
	all[t->getCPU()].tss->ioMapOffset = TSS::IO_MAP_OFFSET;
	if(forceCpy || ioMap != all[t->getCPU()].ioMap)
		memcpy(all[t->getCPU()].tss->ioMap,ioMap,TSS::IO_MAP_SIZE / 8);
	/* remove the map from other cpus; we have to copy it again because it has changed */
	if(forceCpy) {
		for(size_t i = 0; i < cpuCount; i++) {
			if(i != t->getCPU() && all[i].ioMap == ioMap)
				all[i].ioMap = NULL;
		}
	}
	all[t->getCPU()].ioMap = ioMap;
}

void GDT::setupSyscalls(A_UNUSED TSS *tss) {
#if defined(__x86_64__)
	CPU::setMSR(CPU::MSR_IA32_STAR,((selector(SEG_UCODE2) - 16) << 48) | (selector(SEG_KCODE) << 32));
	CPU::setMSR(CPU::MSR_IA32_LSTAR,(uint64_t)&syscall_entry);
	CPU::setMSR(CPU::MSR_IA32_FMASK,CPU::EFLAG_IF);
#else
	CPU::setMSR(CPU::MSR_IA32_SYSENTER_CS,SEG_KCODE << 3);
	CPU::setMSR(CPU::MSR_IA32_SYSENTER_EIP,(uint64_t)&syscall_entry);
	CPU::setMSR(CPU::MSR_IA32_SYSENTER_ESP,tss->REG(sp0));
#endif
}

void GDT::setTSS(Desc *gdt,TSS *tss,uintptr_t kstack) {
#if defined(__x86_64__)
	tss->setSP(kstack + PAGE_SIZE - 1 * sizeof(ulong));
	setDesc64(gdt + SEG_TSS,(uintptr_t)tss,sizeof(TSS) - 1,GRANU_BYTES,SYS_TSS,DPL_KERNEL);
#else
	/* leave a bit space for the vm86-segment-registers that will be present at the stack-top
	 * in vm86-mode. This way we can have the same interrupt-stack for all processes */
	tss->setSP(kstack + PAGE_SIZE - 2 * sizeof(ulong));
	setDesc(gdt + SEG_TSS,(uintptr_t)tss,sizeof(TSS) - 1,GRANU_BYTES,SYS_TSS,DPL_KERNEL);
#endif
}

void GDT::setDesc(Desc *d,uintptr_t address,size_t limit,uint8_t granu,uint8_t type,uint8_t dpl) {
	d->addrLow = address & 0xFFFF;
	d->addrMiddle = (address >> 16) & 0xFF;
	d->addrHigh = (address >> 24) & 0xFF;
	d->limitLow = limit & 0xFFFF;
	d->limitHigh = (limit >> 16) & 0xF;
	d->present = 1;
#if defined(__x86_64__)
	d->size = SIZE_16;
	d->bits = 1;
#else
	d->size = SIZE_32;
	d->bits = 0;
#endif
	d->dpl = dpl;
	d->granularity = granu;
	d->type = type;
}

#if defined(__x86_64__)
void GDT::setDesc64(Desc *d,uintptr_t address,size_t limit,uint8_t granu,uint8_t type,uint8_t dpl) {
	Desc64 *d64 = (Desc64*)d;
	setDesc(d64,address,limit,granu,type,dpl);
	d64->addrUpper = address >> 32;
}
#endif

void GDT::print(OStream &os) {
	os.writef("GDTs:\n");
	for(auto cpu = SMP::begin(); cpu != SMP::end(); ++cpu) {
		Desc *gdt = (Desc*)all[cpu->id].gdt.offset;
		os.writef("\tGDT of CPU %d\n",cpu->id);
		if(gdt) {
			for(size_t i = 0; i < GDT_ENTRY_COUNT; i++) {
				os.writef("\t\t%d: address=%02x%02x:%04x, limit=%02x%04x, type=%02x\n",
						i,gdt[i].addrHigh,gdt[i].addrMiddle,gdt[i].addrLow,
						gdt[i].limitHigh,gdt[i].limitLow,gdt[i].type);
			}
		}
		else
			os.writef("\t\t-\n");
	}
}
