/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/machine/timer.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/swap.h>
#include <sys/mem/vmm.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/elf.h>
#include <sys/task/uenv.h>
#include <sys/vfs/node.h>
#include <sys/multiboot.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/config.h>
#include <errors.h>
#include <string.h>
#include <assert.h>

#define MAX_ARG_COUNT			8
#define MAX_ARG_LEN				64
#define CHECK_FLAG(flags,bit)	(flags & (1 << bit))

static const char **mboot_parseArgs(const char *line,int *argc);

extern uintptr_t KernelStart;
static sMultiBoot *mb;
static bool loadedMods = false;

void mboot_init(sMultiBoot *mbp) {
	size_t i;
	sModule *mod;
	int argc;
	const char **argv;

	/* save the multiboot-structure
	 * (change to 0xC...0 since we get the address at 0x0...0 from GRUB) */
	mb = (sMultiBoot*)((uintptr_t)mbp | KERNEL_AREA_V_ADDR);

	/* change the address of the pointers in the structure, too */
	mb->cmdLine = (char*)((uintptr_t)mb->cmdLine | KERNEL_AREA_V_ADDR);
	mb->modsAddr = (sModule*)((uintptr_t)mb->modsAddr | KERNEL_AREA_V_ADDR);
	mb->mmapAddr = (sMemMap*)((uintptr_t)mb->mmapAddr | KERNEL_AREA_V_ADDR);
	mb->drivesAddr = (sDrive*)((uintptr_t)mb->drivesAddr | KERNEL_AREA_V_ADDR);
	mod = mb->modsAddr;
	for(i = 0; i < mb->modsCount; i++) {
		mod->modStart |= KERNEL_AREA_V_ADDR;
		mod->modEnd |= KERNEL_AREA_V_ADDR;
		mod->name = (char*)((uintptr_t)mod->name | KERNEL_AREA_V_ADDR);
		mod++;
	}

	/* parse the boot parameter */
	argv = mboot_parseArgs(mb->cmdLine,&argc);
	conf_parseBootParams(argc,argv);
}

const sMultiBoot *mboot_getInfo(void) {
	return mb;
}

size_t mboot_getKernelSize(void) {
	uintptr_t start = (uintptr_t)&KernelStart | KERNEL_AREA_V_ADDR;
	uintptr_t end = mb->modsAddr[0].modStart;
	return end - start;
}

size_t mboot_getModuleSize(void) {
	uintptr_t start = mb->modsAddr[0].modStart;
	uintptr_t end = mb->modsAddr[mb->modsCount - 1].modEnd;
	return end - start;
}

void mboot_loadModules(sIntrptStackFrame *stack) {
	size_t i;
	tPid pid;
	tInodeNo nodeNo;
	sModule *mod = mb->modsAddr;

	/* it's not good to do this twice.. */
	if(loadedMods)
		return;

	/* start idle-thread */
	if(proc_startThread(0,NULL) == thread_getRunning()->tid) {
		thread_idle();
		util_panic("Idle returned");
	}

	loadedMods = true;
	for(i = 0; i < mb->modsCount; i++) {
		/* parse args */
		int argc;
		const char **argv = mboot_parseArgs(mod->name,&argc);
		if(argc < 2)
			util_panic("Invalid arguments for multiboot-module: %s\n",mod->name);

		/* clone proc */
		pid = proc_getFreePid();
		if(pid == INVALID_PID)
			util_panic("No free process-slots");

		if(proc_clone(pid,0)) {
			sStartupInfo info;
			size_t argSize = 0;
			char *argBuffer = NULL;
			sProc *p = proc_getRunning();
			/* remove regions (except stack) */
			proc_removeRegions(p,false);
			/* now load module */
			memcpy(p->command,argv[0],strlen(argv[0]) + 1);
			if(elf_loadFromMem((void*)mod->modStart,mod->modEnd - mod->modStart,&info) < 0)
				util_panic("Loading multiboot-module %s failed",p->command);
			/* build args */
			argc = proc_buildArgs(argv,&argBuffer,&argSize,false);
			if(argc < 0)
				util_panic("Building args for multiboot-module %s failed: %d",p->command,argc);
			/* no dynamic linking here */
			p->entryPoint = info.progEntry;
			if(!uenv_setupProc(stack,argc,argBuffer,argSize,&info,info.progEntry))
				util_panic("Unable to setup user-stack for multiboot module %s",p->command);
			kheap_free(argBuffer);
			/* we don't want to continue the loop ;) */
			return;
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
	if(proc_startThread(0,NULL) == thread_getRunning()->tid) {
		swap_start();
		util_panic("Swapper reached this");
	}

	/* create the vm86-task */
	assert(vm86_create() == 0);
}

size_t mboot_getUsableMemCount(void) {
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

static const char **mboot_parseArgs(const char *line,int *argc) {
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


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void mboot_dbg_print(void) {
	size_t x;
	sMemMap *mmap;
	vid_printf("MultiBoot-Structure:\n---------------------\nflags=0x%x\n",mb->flags);
	if(CHECK_FLAG(mb->flags,0)) {
		vid_printf("memLower=%d KB, memUpper=%d KB\n",mb->memLower,mb->memUpper);
	}
	if(CHECK_FLAG(mb->flags,1)) {
		vid_printf("biosDriveNumber=%2X, part1=%2X, part2=%2X, part3=%2X\n",
			(uintptr_t)mb->bootDevice.drive,(uintptr_t)mb->bootDevice.partition1,
			(uintptr_t)mb->bootDevice.partition2,(uintptr_t)mb->bootDevice.partition3);
	}
	if(CHECK_FLAG(mb->flags,2)) {
		vid_printf("cmdLine=%s\n",mb->cmdLine);
	}
	if(CHECK_FLAG(mb->flags,3)) {
		size_t i;
		sModule *mod = mb->modsAddr;
		vid_printf("modsCount=%d:\n",mb->modsCount);
		for(i = 0; i < mb->modsCount; i++) {
			vid_printf("\t%s (0x%x .. 0x%x)\n",mod->name ? mod->name : "<NULL>",
					mod->modStart,mod->modEnd);
			mod++;
		}
	}
	if(CHECK_FLAG(mb->flags,4)) {
		vid_printf("a.out: tabSize=%d, strSize=%d, addr=0x%x\n",mb->syms.aDotOut.tabSize,mb->syms.aDotOut.strSize,
			mb->syms.aDotOut.addr);
	}
	else if(CHECK_FLAG(mb->flags,5)) {
		vid_printf("ELF: num=%d, size=%d, addr=0x%x, shndx=0x%x\n",mb->syms.ELF.num,mb->syms.ELF.size,
			mb->syms.ELF.addr,mb->syms.ELF.shndx);
	}
	if(CHECK_FLAG(mb->flags,6)) {
		vid_printf("mmapLength=%d, mmapAddr=0x%x\n",mb->mmapLength,mb->mmapAddr);
		vid_printf("memory-map:\n");
		x = 0;
		for(mmap = (sMemMap*)mb->mmapAddr;
			(uintptr_t)mmap < (uintptr_t)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
			if(mmap != NULL) {
				vid_printf("\t%d: addr=0x%08x, size=0x%08x, type=%s\n",
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

#endif
