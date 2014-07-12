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

#include <common.h>
#include <mem/pagedir.h>
#include <mem/cache.h>
#include <mem/virtmem.h>
#include <mem/copyonwrite.h>
#include <mem/dynarray.h>
#include <arch/mmix/mem/addrspace.h>
#include <task/proc.h>
#include <task/thread.h>
#include <task/sched.h>
#include <task/elf.h>
#include <task/uenv.h>
#include <task/timer.h>
#include <task/smp.h>
#include <task/terminator.h>
#include <vfs/node.h>
#include <vfs/vfs.h>
#include <vfs/openfile.h>
#include <log.h>
#include <cpu.h>
#include <config.h>
#include <boot.h>
#include <util.h>
#include <video.h>
#include <string.h>

static const BootTask tasks[] = {
	{"Parsing bootinfo...",Boot::parseBootInfo},
	{"Parsing cmdline...",Boot::parseCmdline},
	{"Initializing physical memory-management...",PhysMem::init},
	{"Initializing address spaces...",AddressSpace::init},
	{"Initializing paging...",PageDir::init},
	{"Preinit processes...",Proc::preinit},
	{"Initializing dynarray...",DynArray::init},
	{"Initializing SMP...",SMP::init},
	{"Initializing timer...",Timer::init},
	{"Initializing VFS...",VFS::init},
	{"Initializing processes...",Proc::init},
	{"Initializing scheduler...",Sched::init},
	{"Creating MB module files...",Boot::createModFiles},
	{"Start logging to VFS...",Log::vfsIsReady},
};
BootTaskList Boot::taskList(tasks,ARRAY_SIZE(tasks));

static Boot::Module mods[BOOT_PROG_MAX];
static Boot::MemMap mmap;
static BootInfo *binfo;
static bool initialized = false;

void Boot::archStart(void *nfo) {
	binfo = (BootInfo*)nfo;
	CPU::setSpeed(binfo->cpuHz);
}

void Boot::parseBootInfo() {
	info.cmdLine = (char*)binfo->progs[0].command;
	info.modCount = binfo->progCount - 1;
	info.mods = mods;
	for(size_t i = 1; i < binfo->progCount; ++i) {
		info.mods[i - 1].name = (char*)binfo->progs[i].command;
		info.mods[i - 1].phys = binfo->progs[i].start & ~DIR_MAP_AREA;
		info.mods[i - 1].size = binfo->progs[i].size;
		info.mods[i - 1].virt = binfo->progs[i].start;
	}
	/* mark everything behind the modules as free */
	Module *last = info.mods + info.modCount - 1;
	mmap.baseAddr = ROUND_PAGE_UP(last->phys + last->size);
	mmap.size = binfo->memSize - mmap.baseAddr;
	mmap.type = 1;
	info.mmapCount = 1;
	info.mmap = &mmap;
}

int Boot::init(A_UNUSED IntrptStackFrame *stack) {
	if(initialized)
		return -EEXIST;

	if(unittests != NULL)
		unittests();

	/* we have to do the thread-starting here because we can only start threads when coming from
	 * userland via trap */

	/* start idle-thread */
	Proc::startThread((uintptr_t)&thread_idle,T_IDLE,NULL);
	/* start termination-thread */
	Proc::startThread((uintptr_t)&Terminator::start,0,NULL);
	initialized = true;
	return 0;
}
