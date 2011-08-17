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
#include <sys/task/timer.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/swap.h>
#include <sys/mem/vmm.h>
#include <sys/mem/cow.h>
#include <sys/mem/sharedmem.h>
#include <sys/mem/dynarray.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/sched.h>
#include <sys/task/elf.h>
#include <sys/task/uenv.h>
#include <sys/task/smp.h>
#include <sys/vfs/node.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/request.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/driver.h>
#include <sys/vfs/real.h>
#include <sys/vfs/info.h>
#include <sys/log.h>
#include <sys/boot.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/cpu.h>
#include <sys/config.h>
#include <errors.h>
#include <string.h>
#include <assert.h>

#define MAX_ARG_COUNT			8
#define MAX_ARG_LEN				64
#define CHECK_FLAG(flags,bit)	(flags & (1 << bit))

static const char **boot_parseArgs(const char *line,int *argc);

extern uintptr_t KernelStart;
static sBootInfo *mb;
static bool loadedMods = false;

void boot_init(sBootInfo *mbp,bool logToVFS) {
	size_t i;
	sModule *mod;
	int argc;
	const char **argv;

	/* the first thing we've to do is set up the page-dir and page-table for the kernel and so on
	 * and "correct" the GDT */
	paging_init();
	gdt_init();

	/* save the multiboot-structure
	 * (change to 0xC...0 since we get the address at 0x0...0 from GRUB) */
	mb = (sBootInfo*)((uintptr_t)mbp | KERNEL_START);

	/* change the address of the pointers in the structure, too */
	mb->cmdLine = (char*)((uintptr_t)mb->cmdLine | KERNEL_START);
	mb->modsAddr = (sModule*)((uintptr_t)mb->modsAddr | KERNEL_START);
	mb->mmapAddr = (sMemMap*)((uintptr_t)mb->mmapAddr | KERNEL_START);
	mb->drivesAddr = (sDrive*)((uintptr_t)mb->drivesAddr | KERNEL_START);
	mod = mb->modsAddr;
	for(i = 0; i < mb->modsCount; i++) {
		mod->modStart |= KERNEL_START;
		mod->modEnd |= KERNEL_START;
		mod->name = (char*)((uintptr_t)mod->name | KERNEL_START);
		mod++;
	}

	/* parse the boot parameter */
	argv = boot_parseArgs(mb->cmdLine,&argc);
	conf_parseBootParams(argc,argv);

	/* init video and serial-ports */
	vid_init();
	ser_init();
	proc_preinit();

	vid_printf("GDT exchanged, paging enabled, video initialized");
	vid_printf("\033[co;2]%|s\033[co]","DONE");

#if DEBUGGING
	boot_print();
#endif

	/* mm */
	vid_printf("Initializing physical memory-management...");
	pmem_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* paging */
	vid_printf("Initializing paging...");
	paging_mapKernelSpace();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* cpu */
	vid_printf("Detecting CPU...");
	cpu_detect();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* smp */
	vid_printf("Initializing SMP...");
	dyna_init();
	smp_init();
	gdt_init_bsp();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* fpu */
	vid_printf("Initializing FPU...");
	fpu_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* vfs */
	vid_printf("Initializing VFS...");
	vfs_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* processes */
	vid_printf("Initializing process-management...");
	ev_init();
	proc_init();
	paging_exchangePDir(proc_getPageDir()->own);
	sched_init();
	/* the process and thread-stuff has to be ready, too ... */
	if(logToVFS)
		log_vfsIsReady();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* vmm */
	vid_printf("Initializing virtual memory management...");
	vmm_init();
	cow_init();
	shm_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* idt */
	vid_printf("Initializing IDT...");
	intrpt_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* timer */
	vid_printf("Initializing timer...");
	timer_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* signals */
	vid_printf("Initializing signal-handling...");
	sig_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

#if DEBUGGING
	vid_printf("%d free frames (%d KiB)\n",pmem_getFreeFrames(MM_CONT | MM_DEF),
			pmem_getFreeFrames(MM_CONT | MM_DEF) * PAGE_SIZE / K);
#endif
}

const sBootInfo *boot_getInfo(void) {
	return mb;
}

size_t boot_getKernelSize(void) {
	uintptr_t start = (uintptr_t)&KernelStart | KERNEL_START;
	uintptr_t end = mb->modsAddr[0].modStart;
	return end - start;
}

size_t boot_getModuleSize(void) {
	uintptr_t start = mb->modsAddr[0].modStart;
	uintptr_t end = mb->modsAddr[mb->modsCount - 1].modEnd;
	return end - start;
}

size_t boot_getUsableMemCount(void) {
	sMemMap *mmap;
	size_t size = 0;
	for(mmap = mb->mmapAddr;
		(uintptr_t)mmap < (uintptr_t)mb->mmapAddr + mb->mmapLength;
		mmap = (sMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
		if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE)
			size += mmap->length;
	}
	return size;
}

int boot_loadModules(sIntrptStackFrame *stack) {
	UNUSED(stack);
	size_t i;
	pid_t pid;
	inode_t nodeNo;
	sModule *mod = mb->modsAddr;

	/* it's not good to do this twice.. */
	if(loadedMods)
		return 0;

	loadedMods = true;
	for(i = 0; i < mb->modsCount; i++) {
		/* parse args */
		int argc;
		const char **argv = boot_parseArgs(mod->name,&argc);
		if(argc < 2)
			util_panic("Invalid arguments for multiboot-module: %s\n",mod->name);

		/* clone proc */
		pid = proc_getFreePid();
		if(pid == INVALID_PID)
			util_panic("No free process-slots");

		if(proc_clone(pid,0)) {
			int res = proc_exec(argv[0],argv,(void*)mod->modStart,mod->modEnd - mod->modStart);
			if(res < 0)
				util_panic("Unable to exec boot-program %s: %d\n",argv[0],res);
			/* we don't want to continue ;) */
			return 0;
		}

		/* wait until the driver is registered */
		vid_printf("Loading '%s'...\n",argv[0]);
		/* don't create a pipe- or driver-usage-node here */
		while(vfs_node_resolvePath(argv[1],&nodeNo,NULL,VFS_NOACCESS) < 0) {
			/* Note that we HAVE TO sleep here because we may be waiting for ata and fs is not
			 * started yet. I.e. if ata calls sleep() there is no other runnable thread (except
			 * idle, but its just chosen if nobody else wants to run), so that we wouldn't make
			 * a switch but stay here for ever (and get no timer-interrupts to wakeup ata) */
			timer_sleepFor(thread_getRunning()->tid,20);
			thread_switch();
		}

		mod++;
	}

	/* start the swapper-thread. it will never return */
	proc_startThread((uintptr_t)&swap_start,0,NULL);

	/* if not requested otherwise, from now on, print only to log */
	if(!conf_get(CONF_LOG2SCR))
		vid_setTargets(TARGET_LOG);
	return 0;
}

static const char **boot_parseArgs(const char *line,int *argc) {
	static char argvals[MAX_ARG_COUNT][MAX_ARG_LEN];
	static char *args[MAX_ARG_COUNT];
	size_t i = 0,j = 0;
	args[0] = argvals[0];
	while(*line) {
		if(*line == ' ') {
			if(args[j][0]) {
				if(j + 1 >= MAX_ARG_COUNT)
					break;
				args[j][i] = '\0';
				j++;
				i = 0;
				args[j] = argvals[j];
			}
		}
		else if(i < MAX_ARG_LEN)
			args[j][i++] = *line;
		line++;
	}
	*argc = j + 1;
	args[j][i] = '\0';
	return (const char**)args;
}

void boot_print(void) {
	size_t x;
	sMemMap *mmap;
	vid_printf("MultiBoot-Structure:\n---------------------\nflags=0x%x\n",mb->flags);
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
			vid_printf("\t%s [%p .. %p]\n",mod->name ? mod->name : "<NULL>",
					mod->modStart,mod->modEnd);
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
				vid_printf("\t%d: addr=%p, size=0x%08x, type=%s\n",
						x,(uintptr_t)mmap->baseAddr,(size_t)mmap->length,
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

	vid_printf("---------------------\n");
}
