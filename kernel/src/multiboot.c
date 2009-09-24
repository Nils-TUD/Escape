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
#include <mem/paging.h>
#include <mem/kheap.h>
#include <task/proc.h>
#include <task/thread.h>
#include <task/elf.h>
#include <vfs/node.h>
#include <multiboot.h>
#include <video.h>
#include <util.h>
#include <errors.h>
#include <string.h>

#define MB_PR(fmt,...)			/*vid_printf("[MB] " #fmt "\n",## __VA_ARGS__)*/

#define CHECK_FLAG(flags,bit)	(flags & (1 << bit))

sMultiBoot *mb;
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
	mod = mb->modsAddr;
	for(i = 0; i < mb->modsCount; i++) {
		mod->modStart |= KERNEL_AREA_V_ADDR;
		mod->modEnd |= KERNEL_AREA_V_ADDR;
		mod->name = (char*)((u32)mod->name | KERNEL_AREA_V_ADDR);
		mod++;
	}
}

void mboot_loadModules(sIntrptStackFrame *stack) {
	u32 i,entryPoint;
	tPid pid;
	sProc *p;
	char *name,*space,*service;
	tInodeNo nodeNo;
	sModule *mod = mb->modsAddr;

	/* it's not good to do this twice.. */
	if(loadedMods)
		return;

	MB_PR("We're going to load %d modules",mb->modsCount);

	loadedMods = true;
	for(i = 0; i < mb->modsCount; i++) {
		name = mod->name;
		space = strchr(name,' ');
		service = space + 1;
		service[-1] = '\0';
		space = strchr(service,' ');
		if(space)
			space[0] = '\0';

		MB_PR("Loading module %d: '%s' '%s'",i,name,service);

		/* clone proc */
		pid = proc_getFreePid();
		if(pid == INVALID_PID)
			util_panic("No free process-slots");

		MB_PR("Cloning");
		if(proc_clone(pid)) {
			/* build args */
			s32 argc;
			u32 argSize = 0;
			char *argBuffer = NULL;
			char *args[] = {NULL,NULL,NULL,NULL};
			args[0] = name;
			MB_PR("We're the child");
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
			MB_PR("Arguments (%d): '%s' '%s' '%s'",argc,args[0],args[1] ? args[1] : "",
					args[2] ? args[2] : "");
			/* we'll reach this as soon as the scheduler has chosen the created process */
			p = proc_getRunning();
			MB_PR("Removing data-pages");
			/* remove data-pages */
			proc_changeSize(-p->dataPages,CHG_DATA);
			MB_PR("Loading from mem");
			/* now load service */
			memcpy(p->command,name,strlen(name) + 1);
			entryPoint = elf_loadFromMem((u8*)mod->modStart,mod->modEnd - mod->modStart);
			if((s32)entryPoint < 0)
				util_panic("Loading multiboot-module %s failed",p->command);
			MB_PR("Building arguments");
			argc = proc_buildArgs(args,&argBuffer,&argSize,false);
			if(argc < 0)
				util_panic("Building args for multiboot-module %s failed: %d",p->command,argc);
			MB_PR("Setup stack");
			proc_setupUserStack(stack,argc,argBuffer,argSize);
			proc_setupStart(stack,entryPoint);
			kheap_free(argBuffer);
			MB_PR("We're done :)");
			/* we don't want to continue the loop ;) */
			return;
		}

		/* wait until the service is registered */
		vid_printf("Waiting for '%s'",service);
		/* don't wait for ATA, since it doesn't register a service but multiple drivers depending
		 * on the available drives and partitions */
		/* TODO better solution? */
		if(strcmp(service,"/services/ata") != 0) {
			/* don't create a pipe- or service-usage-node here */
			while(vfsn_resolvePath(service,&nodeNo,NULL,VFS_NOACCESS) < 0)
				thread_switchInKernel();
		}
		vid_toLineEnd(SSTRLEN("DONE"));
		vid_printf("\033[co;2]DONE\033[co]");

		mod++;
	}
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
		vid_printf("biosDriveNumber=%d, part1=%d, part2=%d, part3=%d\n",mb->bootDevice.drive,
			mb->bootDevice.partition1,mb->bootDevice.partition2,mb->bootDevice.partition3);
	}
	if(CHECK_FLAG(mb->flags,2)) {
		vid_printf("cmdLine=%s\n",mb->cmdLine);
	}
	if(CHECK_FLAG(mb->flags,3)) {
		u32 i;
		sModule *mod = mb->modsAddr;
		vid_printf("modsCount=%d:\n",mb->modsCount);
		for(i = 0; i < mb->modsCount; i++) {
			vid_printf("\tname=%s, start=0x%x, end=0x%x\n",mod->name ? mod->name : "<NULL>",
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
		vid_printf("Available memory:\n");
		x = 0;
		for(mmap = (sMemMap*)mb->mmapAddr;
			(u32)mmap < (u32)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((u32)mmap + mmap->size + sizeof(mmap->size))) {
			if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE) {
				vid_printf("\t%d: addr=0x%08x, size=0x%08x, type=0x%08x\n",
						x,(u32)mmap->baseAddr,(u32)mmap->length,mmap->type);
				x++;
			}
		}
	}

	vid_printf("---------------------\n");
}

#endif
