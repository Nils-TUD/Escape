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

#include <arch/x86/acpi.h>
#include <arch/x86/fpu.h>
#include <arch/x86/gdt.h>
#include <arch/x86/idt.h>
#include <arch/x86/ioapic.h>
#include <arch/x86/mtrr.h>
#include <arch/x86/pic.h>
#include <arch/x86/rtc.h>
#include <arch/x86/serial.h>
#include <mem/cache.h>
#include <mem/copyonwrite.h>
#include <mem/dynarray.h>
#include <mem/kasan.h>
#include <mem/pagedir.h>
#include <mem/physmemareas.h>
#include <mem/virtmem.h>
#include <task/elf.h>
#include <task/proc.h>
#include <task/sched.h>
#include <task/smp.h>
#include <task/terminator.h>
#include <task/thread.h>
#include <task/timer.h>
#include <task/uenv.h>
#include <vfs/file.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <boot.h>
#include <common.h>
#include <config.h>
#include <cppsupport.h>
#include <cpu.h>
#include <errno.h>
#include <log.h>
#include <string.h>
#include <util.h>
#include <video.h>

static void mapModules();

static const BootTask tasks[] = {
	{"Parsing multiboot info...",Boot::parseBootInfo},
	{"Initializing PageDir...",PageDir::init},
	{"Initializing GDT...",GDT::init},
	{"Parsing cmdline...",Boot::parseCmdline},
	{"Pre-initializing processes...",Proc::preinit},
	{"Initializing physical memory management...",PhysMem::init},
	{"Initializing kernel address sanitizer...",KASan::init},
	{"Map modules...",mapModules},
	{"Initializing dynarray...",DynArray::init},
	{"Initializing LAPIC...",LAPIC::init},
	{"Initializing ACPI...",ACPI::init},
	{"Initializing SMP...",SMP::init},
	{"Initializing GDT for BSP...",GDT::initBSP},
	{"Initializing CPU...",CPU::detect},
	{"Initializing MTRRs...",MTRR::init},
	{"Initializing FPU...",FPU::init},
	{"Initializing RTC...",RTC::init},
	{"Initializing timer...",Timer::init},
	{"Initializing VFS...",VFS::init},
	{"Initializing scheduler...",Sched::init},
	{"Initializing processes...",Proc::init},
	{"Creating ACPI files...",ACPI::createFiles},
	{"Creating MB module files...",Boot::createModFiles},
	{"Start logging to VFS...",Log::vfsIsReady},
	{"Initializing interrupts...",Interrupts::init},
	{"Initializing IDT...",IDT::init},
};
BootTaskList Boot::taskList(tasks,ARRAY_SIZE(tasks));

static char mbbuf[PAGE_SIZE];
static size_t mbbufpos = 0;
static MultiBootInfo *mbinfo;
static bool initialized = false;

static void *copyMBInfo(uintptr_t info,size_t len) {
	void *res = mbbuf + mbbufpos;
	if(mbbufpos + len > sizeof(mbbuf))
		Util::panic("Multiboot-buffer too small");
	memcpy(mbbuf + mbbufpos,(const void*)info,len);
	mbbufpos += len;
	return res;
}

void Boot::archStart(void *nfo) {
	mbinfo = (MultiBootInfo*)nfo;
	taskList.moduleCount = mbinfo->modsCount;
	/* setup serial before printing anything to the serial line */
	Serial::init();
}

void Boot::parseBootInfo() {
	BootModule *mbmod;
	/* copy mb-stuff into our own datastructure */
	info.cmdLine = (char*)copyMBInfo(mbinfo->cmdLine,strlen((char*)(uintptr_t)mbinfo->cmdLine) + 1);
	info.modCount = mbinfo->modsCount;

	/* copy memory map */
	info.mmap = (MemMap*)(mbbuf + mbbufpos);
	info.mmapCount = 0;
	for(BootMemMap *mmap = (BootMemMap*)(uintptr_t)mbinfo->mmapAddr;
			(uintptr_t)mmap < (uintptr_t)mbinfo->mmapAddr + mbinfo->mmapLength;
			mmap = (BootMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
		info.mmap[info.mmapCount].baseAddr = mmap->baseAddr;
		info.mmap[info.mmapCount].size = mmap->length;
		info.mmap[info.mmapCount].type = mmap->type;

		/* make the first megabyte reserved. this way, the userland can map it */
		if(mmap->baseAddr < 1024 * 1024)
			info.mmap[info.mmapCount].type = 2;

		mbbufpos += sizeof(MemMap);
		info.mmapCount++;
	}

	/* copy modules (do that here because we can't access the multiboot-info afterwards anymore) */
	info.mods = (Module*)(mbbuf + mbbufpos);
	mbbufpos += sizeof(Module) * mbinfo->modsCount;
	mbmod = (BootModule*)(uintptr_t)mbinfo->modsAddr;
	for(size_t i = 0; i < mbinfo->modsCount; i++) {
		info.mods[i].phys = mbmod->modStart;
		info.mods[i].virt = 0;
		info.mods[i].size = mbmod->modEnd - mbmod->modStart;
		info.mods[i].name = (char*)copyMBInfo(mbmod->name,strlen((char*)(uintptr_t)mbmod->name) + 1);
		mbmod++;
	}
}

static void mapModules() {
	/* make modules accessible */
	for(auto mod = Boot::modsBegin(); mod != Boot::modsEnd(); ++mod)
		mod->virt = PageDir::makeAccessible(mod->phys,BYTES_2_PAGES(mod->size));
}

int Boot::init(A_UNUSED IntrptStackFrame *stack) {
	if(initialized)
		return -EEXIST;

	if(unittests != NULL)
		unittests();
	initialized = true;
	return 0;
}
