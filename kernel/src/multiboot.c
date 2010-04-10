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

#include <common.h>
#include <machine/timer.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <mem/swap.h>
#include <mem/vmm.h>
#include <task/proc.h>
#include <task/thread.h>
#include <task/elf.h>
#include <vfs/node.h>
#include <multiboot.h>
#include <video.h>
#include <util.h>
#include <config.h>
#include <errors.h>
#include <string.h>
#include <assert.h>

#define CHECK_FLAG(flags,bit)	(flags & (1 << bit))

extern u32 KernelStart;
static sMultiBoot *mb;
static bool loadedMods = false;

void mboot_init(sMultiBoot *mbp) {
	u32 i;
	sModule *mod;

	/* save the multiboot-structure
	 * (change to 0xC...0 since we get the address at 0x0...0 from GRUB) */
	mb = (sMultiBoot*)((u32)mbp | KERNEL_AREA_V_ADDR);

	/* change the address of the pointers in the structure, too */
	mb->cmdLine = (char*)((u32)mb->cmdLine | KERNEL_AREA_V_ADDR);
	mb->modsAddr = (sModule*)((u32)mb->modsAddr | KERNEL_AREA_V_ADDR);
	mb->mmapAddr = (sMemMap*)((u32)mb->mmapAddr | KERNEL_AREA_V_ADDR);
	mb->drivesAddr = (sDrive*)((u32)mb->drivesAddr | KERNEL_AREA_V_ADDR);
	mod = mb->modsAddr;
	for(i = 0; i < mb->modsCount; i++) {
		mod->modStart |= KERNEL_AREA_V_ADDR;
		mod->modEnd |= KERNEL_AREA_V_ADDR;
		mod->name = (char*)((u32)mod->name | KERNEL_AREA_V_ADDR);
		mod++;
	}

	/* parse the boot parameter */
	conf_parseBootParams(mb->cmdLine);
}

const sMultiBoot *mboot_getInfo(void) {
	return mb;
}

u32 mboot_getKernelSize(void) {
	u32 start = (u32)&KernelStart | KERNEL_AREA_V_ADDR;
	u32 end = mb->modsAddr[0].modStart;
	return end - start;
}

u32 mboot_getModuleSize(void) {
	u32 start = mb->modsAddr[0].modStart;
	u32 end = mb->modsAddr[mb->modsCount - 1].modEnd;
	return end - start;
}

void mboot_loadModules(sIntrptStackFrame *stack) {
	u32 i,entryPoint;
	tPid pid;
	sProc *p;
	char *name,*space,*driver;
	tInodeNo nodeNo;
	sModule *mod = mb->modsAddr;

	/* it's not good to do this twice.. */
	if(loadedMods)
		return;

	loadedMods = true;
	for(i = 0; i < mb->modsCount; i++) {
		name = mod->name;
		space = strchr(name,' ');
		driver = space + 1;
		driver[-1] = '\0';
		space = strchr(driver,' ');
		if(space)
			space[0] = '\0';

		/* clone proc */
		pid = proc_getFreePid();
		if(pid == INVALID_PID)
			util_panic("No free process-slots");

		if(proc_clone(pid,false)) {
			/* build args */
			s32 argc;
			u32 argSize = 0;
			char *argBuffer = NULL;
			char *args[] = {NULL,NULL,NULL,NULL};
			args[0] = name;
			/* just two arguments supported here */
			if(space != NULL) {
				char *nnspace = strchr(space + 1,' ');
				if(nnspace) {
					*nnspace = '\0';
					if(strchr(nnspace + 1,' ') != NULL)
						util_panic("Invalid arguments to multiboot-module %s",name);
					args[2] = nnspace + 1;
				}
				args[1] = space + 1;
			}
			/* we'll reach this as soon as the scheduler has chosen the created process */
			p = proc_getRunning();
			/* remove regions (except stack) */
			vmm_removeAll(p,false);
			/* now load driver */
			memcpy(p->command,name,strlen(name) + 1);
			entryPoint = elf_loadFromMem((u8*)mod->modStart,mod->modEnd - mod->modStart);
			if((s32)entryPoint < 0)
				util_panic("Loading multiboot-module %s failed",p->command);
			argc = proc_buildArgs(args,&argBuffer,&argSize,false);
			if(argc < 0)
				util_panic("Building args for multiboot-module %s failed: %d",p->command,argc);
			vassert(proc_setupUserStack(stack,argc,argBuffer,argSize,entryPoint),
					"Unable to setup user-stack for multiboot module %s",p->command);
			proc_setupStart(stack);
			kheap_free(argBuffer);
			/* we don't want to continue the loop ;) */
			return;
		}

		/* wait until the driver is registered */
		vid_printf("Loading '%s'...\n",name);
		/* don't create a pipe- or driver-usage-node here */
		while(vfsn_resolvePath(driver,&nodeNo,NULL,VFS_NOACCESS) < 0) {
			/* Note that we HAVE TO sleep here because we may be waiting for ata and fs is not
			 * started yet. I.e. if ata calls sleep() there is no other runnable thread (except
			 * idle, but its just chosen if nobody else wants to run), so that we wouldn't make
			 * a switch but stay here for ever (and get no timer-interrupts to wakeup ata) */
			timer_sleepFor(thread_getRunning()->tid,20);
			thread_switchNoSigs();
		}

		mod++;
	}

	/* start the swapper-thread. it will never return */
	if(proc_startThread(0,NULL) == 0) {
		swap_start();
		util_panic("Swapper reached this");
	}

	/* create the vm86-task */
	assert(vm86_create() == 0);
}

u32 mboot_getUsableMemCount(void) {
	sMemMap *mmap;
	u32 size = 0;
	for(mmap = mb->mmapAddr;
		(u32)mmap < (u32)mb->mmapAddr + mb->mmapLength;
		mmap = (sMemMap*)((u32)mmap + mmap->size + sizeof(mmap->size))) {
		if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE)
			size += mmap->length;
	}
	return size;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void mboot_dbg_print(void) {
	u32 x;
	sMemMap *mmap;
	vid_printf("MultiBoot-Structure:\n---------------------\nflags=0x%x\n",mb->flags);
	if(CHECK_FLAG(mb->flags,0)) {
		vid_printf("memLower=%d KB, memUpper=%d KB\n",mb->memLower,mb->memUpper);
	}
	if(CHECK_FLAG(mb->flags,1)) {
		vid_printf("biosDriveNumber=%2X, part1=%2X, part2=%2X, part3=%2X\n",
			(u32)mb->bootDevice.drive,(u32)mb->bootDevice.partition1,(u32)mb->bootDevice.partition2,
			(u32)mb->bootDevice.partition3);
	}
	if(CHECK_FLAG(mb->flags,2)) {
		vid_printf("cmdLine=%s\n",mb->cmdLine);
	}
	if(CHECK_FLAG(mb->flags,3)) {
		u32 i;
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
			(u32)mmap < (u32)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((u32)mmap + mmap->size + sizeof(mmap->size))) {
			if(mmap != NULL) {
				vid_printf("\t%d: addr=0x%08x, size=0x%08x, type=%s\n",
						x,(u32)mmap->baseAddr,(u32)mmap->length,
						mmap->type == MMAP_TYPE_AVAILABLE ? "free" : "used");
				x++;
			}
		}
	}
	if(CHECK_FLAG(mb->flags,7) && mb->drivesLength > 0) {
		u32 i;
		sDrive *drive = mb->drivesAddr;
		vid_printf("Drives: (size=%u)\n",mb->drivesLength);
		for(x = 0, i = 0; x < mb->drivesLength; x += drive->size) {
			vid_printf("\t%d: no=%x, mode=%x, cyl=%u, heads=%u, sectors=%u\n",
					i,(u32)drive->number,(u32)drive->mode,(u32)drive->cylinders,(u32)drive->heads,
					(u32)drive->sectors);
		}
	}

	vid_printf("---------------------\n");
}

#endif
