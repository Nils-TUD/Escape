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
#include <sys/arch/i586/task/vm86.h>
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/serial.h>
#include <sys/arch/i586/idt.h>
#include <sys/arch/i586/pic.h>
#include <sys/task/timer.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/copyonwrite.h>
#include <sys/mem/dynarray.h>
#include <sys/mem/physmemareas.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/sched.h>
#include <sys/task/elf.h>
#include <sys/task/uenv.h>
#include <sys/task/smp.h>
#include <sys/task/terminator.h>
#include <sys/vfs/file.h>
#include <sys/vfs/node.h>
#include <sys/vfs/vfs.h>
#include <sys/log.h>
#include <sys/boot.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/cpu.h>
#include <sys/config.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#define MAX_MOD_COUNT			10
#define CHECK_FLAG(flags,bit)	(flags & (1 << bit))
#define PHYS2VIRT(x)			((void*)((uintptr_t)x + KERNEL_AREA))

static const sBootTask tasks[] = {
	{"Initializing dynarray...",DynArray::init},
	{"Initializing SMP...",SMP::init},
	{"Initializing GDT...",GDT::init_bsp},
	{"Initializing CPU...",CPU::detect},
	{"Initializing FPU...",FPU::init},
	{"Initializing VFS...",vfs_init},
	{"Initializing event system...",Event::init},
	{"Initializing processes...",Proc::init},
	{"Initializing scheduler...",Sched::init},
	{"Initializing terminator...",Terminator::init},
	{"Start logging to VFS...",log_vfsIsReady},
	{"Initializing copy-on-write...",CopyOnWrite::init},
	{"Initializing interrupts...",Interrupts::init},
	{"Initializing PIC...",PIC::init},
	{"Initializing IDT...",IDT::init},
	{"Initializing timer...",Timer::init},
	{"Initializing signal handling...",Signals::init},
};
sBootTaskList bootTaskList(tasks,ARRAY_SIZE(tasks));
static uintptr_t physModAddrs[MAX_MOD_COUNT];
static char mbbuf[PAGE_SIZE];
static size_t mbbufpos = 0;

extern void *_btext;
extern void *_ebss;
static sBootInfo *mb;
static bool loadedMods = false;

static void *boot_copy_mbinfo(const void *info,size_t len) {
	void *res = mbbuf + mbbufpos;
	if(mbbufpos + len > sizeof(mbbuf))
		util_panic("Multiboot-buffer too small");
	memcpy(mbbuf + mbbufpos,PHYS2VIRT(info),len);
	mbbufpos += len;
	return res;
}

void boot_arch_start(sBootInfo *info) {
	size_t i;
	sModule *mod;
	int argc;
	const char **argv;

	/* copy mb-stuff into buffer */
	info = (sBootInfo*)boot_copy_mbinfo(info,sizeof(sBootInfo));
	info->cmdLine = (char*)boot_copy_mbinfo(info->cmdLine,strlen((char*)PHYS2VIRT(info->cmdLine)) + 1);
	info->modsAddr = (sModule*)boot_copy_mbinfo(info->modsAddr,sizeof(sModule) * info->modsCount);
	info->mmapAddr = (sMemMap*)boot_copy_mbinfo(info->mmapAddr,info->mmapLength);
	info->drivesAddr = (sDrive*)boot_copy_mbinfo(info->drivesAddr,info->drivesLength);
	mod = info->modsAddr;
	for(i = 0; i < info->modsCount; i++) {
		mod->name = (char*)boot_copy_mbinfo(mod->name,strlen((char*)PHYS2VIRT(mod->name)) + 1);
		physModAddrs[i] = mod->modStart;
		mod++;
	}
	mb = info;
	if(mb->modsCount > MAX_MOD_COUNT)
		util_panic("Too many modules (max %u)",MAX_MOD_COUNT);
	bootTaskList.moduleCount = mb->modsCount;

	/* set up the page-dir and page-table for the kernel and so on and "correct" the GDT */
	PageDir::init();
	GDT::init();
	Thread::setRunning(NULL);

	/* init basic modules */
	vid_init();
	FPU::preinit();
	Serial::init();

	/* init physical memory and paging */
	Proc::preinit();
	PhysMem::init();
	PageDir::mapKernelSpace();

	/* now map modules */
	mod = mb->modsAddr;
	for(i = 0; i < mb->modsCount; i++) {
		size_t size = mod->modEnd - mod->modStart;
		mod->modStart = PageDir::makeAccessible(mod->modStart,BYTES_2_PAGES(size));
		mod->modEnd = mod->modStart + size;
		mod++;
	}

	/* parse boot parameters */
	argv = boot_parseArgs(mb->cmdLine,&argc);
	Config::parseBootParams(argc,argv);
}

const sBootInfo *boot_getInfo(void) {
	return mb;
}

size_t boot_getKernelSize(void) {
	uintptr_t start = (uintptr_t)&_btext;
	uintptr_t end = (uintptr_t)&_ebss;
	return end - start;
}

size_t boot_getModuleSize(void) {
	if(mb->modsCount == 0)
		return 0;
	uintptr_t start = mb->modsAddr[0].modStart;
	uintptr_t end = mb->modsAddr[mb->modsCount - 1].modEnd;
	return end - start;
}

uintptr_t boot_getModuleRange(const char *name,size_t *size) {
	size_t i;
	sModule *mod = mb->modsAddr;
	for(i = 0; i < mb->modsCount; i++) {
		if(strcmp(mod->name,name) == 0) {
			*size = mod->modEnd - mod->modStart;
			return physModAddrs[i];
		}
		mod++;
	}
	return 0;
}

int boot_loadModules(A_UNUSED IntrptStackFrame *stack) {
	char loadingStatus[256];
	size_t i;
	int child,res;
	inode_t nodeNo;
	sVFSNode *mbmods;
	sModule *mod;

	/* it's not good to do this twice.. */
	if(loadedMods)
		return 0;

	/* create module files */
	res = vfs_node_resolvePath("/system/mbmods",&nodeNo,NULL,0);
	if(res < 0)
		util_panic("Unable to resolve /system/mbmods");
	mbmods = vfs_node_get(nodeNo);
	mod = mb->modsAddr;
	for(i = 0; i < mb->modsCount; i++) {
		char *modname = (char*)Cache::alloc(12);
		itoa(modname,12,i);
		sVFSNode *n = vfs_file_create_for(KERNEL_PID,mbmods,modname,(void*)mod->modStart,
				mod->modEnd - mod->modStart);
		if(!n || vfs_node_chmod(KERNEL_PID,vfs_node_getNo(n),S_IRUSR | S_IRGRP | S_IROTH) != 0)
			util_panic("Unable to create/chmod mbmod-file for '%s'",modname);
		mod++;
	}

	/* load modules */
	loadedMods = true;
	mod = mb->modsAddr;
	for(i = 0; i < mb->modsCount; i++) {
		/* parse args */
		int argc;
		const char **argv = boot_parseArgs(mod->name,&argc);
		/* ignore modules with just a path; this might be a ramdisk */
		if(argc == 1) {
			boot_taskFinished();
			mod++;
			continue;
		}
		if(argc < 2)
			util_panic("Invalid arguments for multiboot-module: %s\n",mod->name);

		strcpy(loadingStatus,"Loading ");
		strcat(loadingStatus,argv[0]);
		strcat(loadingStatus,"...");
		boot_taskStarted(loadingStatus);

		if((child = Proc::clone(P_BOOT)) == 0) {
			res = Proc::exec(argv[0],argv,(void*)mod->modStart,mod->modEnd - mod->modStart);
			if(res < 0)
				util_panic("Unable to exec boot-program %s: %d\n",argv[0],res);
			/* we don't want to continue ;) */
			return 0;
		}
		else if(child < 0)
			util_panic("Unable to clone process for boot-program %s: %d\n",argv[0],child);

		/* wait until the device is registered */
		/* don't create a pipe- or channel-node here */
		while(vfs_node_resolvePath(argv[1],&nodeNo,NULL,VFS_NOACCESS) < 0) {
			/* Note that we HAVE TO sleep here because we may be waiting for ata and fs is not
			 * started yet. I.e. if ata calls sleep() there is no other runnable thread (except
			 * idle, but its just chosen if nobody else wants to run), so that we wouldn't make
			 * a switch but stay here for ever (and get no timer-interrupts to wakeup ata) */
			Timer::sleepFor(Thread::getRunning()->getTid(),20,true);
			Thread::switchAway();
		}

		boot_taskFinished();
		mod++;
	}

	/* start the swapper-thread. it will never return */
	if(PhysMem::canSwap())
		Proc::startThread((uintptr_t)&PhysMem::swapper,0,NULL);
	/* start the terminator */
	Proc::startThread((uintptr_t)&Terminator::start,0,NULL);

	/* if not requested otherwise, from now on, print only to log */
	if(!Config::get(Config::LOG2SCR))
		vid_setTargets(TARGET_LOG);
	return 0;
}

void boot_print(void) {
	size_t x;
	sMemMap *mmap;
	vid_printf("flags=0x%x\n",mb->flags);
	if(CHECK_FLAG(mb->flags,0)) {
		vid_printf("memLower=%d KB, memUpper=%d KB\n",mb->memLower,mb->memUpper);
	}
	if(CHECK_FLAG(mb->flags,1)) {
		vid_printf("biosDriveNumber=%2X, part1=%2X, part2=%2X, part3=%2X\n",
			(uint)mb->bootDevice.drive,(uint)mb->bootDevice.partition1,
			(uint)mb->bootDevice.partition2,(uint)mb->bootDevice.partition3);
	}
	if(CHECK_FLAG(mb->flags,2)) {
		vid_printf("cmdLine=%s\n",mb->cmdLine);
	}
	if(CHECK_FLAG(mb->flags,3)) {
		size_t i;
		sModule *mod = mb->modsAddr;
		vid_printf("modsCount=%d:\n",mb->modsCount);
		for(i = 0; i < mb->modsCount; i++) {
			vid_printf("\t[%zu]: virt: %p..%p, phys: %p..%p\n",i,mod->modStart,mod->modEnd,
					physModAddrs[i],physModAddrs[i] + (mod->modEnd - mod->modStart));
			vid_printf("\t     cmdline: %s\n",mod->name ? mod->name : "<NULL>");
			mod++;
		}
	}
	if(CHECK_FLAG(mb->flags,4)) {
		vid_printf("a.out: tabSize=%d, strSize=%d, addr=0x%x\n",mb->syms.aDotOut.tabSize,
				mb->syms.aDotOut.strSize,mb->syms.aDotOut.addr);
	}
	else if(CHECK_FLAG(mb->flags,5)) {
		vid_printf("ELF: num=%d, size=%d, addr=0x%x, shndx=0x%x\n",mb->syms.ELF.num,mb->syms.ELF.size,
			mb->syms.ELF.addr,mb->syms.ELF.shndx);
	}
	if(CHECK_FLAG(mb->flags,6)) {
		vid_printf("mmapLength=%d, mmapAddr=%p\n",mb->mmapLength,mb->mmapAddr);
		vid_printf("memory-map:\n");
		x = 0;
		for(mmap = (sMemMap*)mb->mmapAddr;
			(uintptr_t)mmap < (uintptr_t)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
			if(mmap != NULL) {
				vid_printf("\t%d: addr=%#012Lx, size=%#012Lx, type=%s\n",
						x,mmap->baseAddr,mmap->length,
						mmap->type == MMAP_TYPE_AVAILABLE ? "free" : "used");
				x++;
			}
		}
	}
	if(CHECK_FLAG(mb->flags,7) && mb->drivesLength > 0) {
		size_t i;
		sDrive *drive = mb->drivesAddr;
		vid_printf("Drives: (size=%u)\n",mb->drivesLength);
		for(x = 0, i = 0; x < mb->drivesLength; x += drive->size) {
			vid_printf("\t%d: no=%x, mode=%x, cyl=%u, heads=%u, sectors=%u\n",
					i,(uint)drive->number,(uint)drive->mode,(uint)drive->cylinders,
					(uint)drive->heads,(uint)drive->sectors);
		}
	}
}
