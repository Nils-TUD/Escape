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
#include <multiboot.h>
#include <gdt.h>
#include <mm.h>
#include <util.h>
#include <paging.h>
#include <proc.h>
#include <intrpt.h>
#include <debug.h>
#include <cpu.h>
#include <elf.h>
#include <kheap.h>
#include <elf.h>
#include <sched.h>
#include <signals.h>
#include <vfs.h>
#include <vfsinfo.h>
#include <vfsreq.h>
#include <vfsdrv.h>
#include <vfsreal.h>
#include <kevent.h>
#include <video.h>
#include <timer.h>
#include <string.h>
#include <fpu.h>

/*
	0x00000000 - 0x000003FF : Real mode interrupt vector table
	0x00000400 - 0x000004FF : BIOS data area
	0x00000500 - 0x00007BFF : Unused
	0x00007C00 - 0x00007DFF : Floppy boot sector is loaded in here
	0x00007E00 - 0x0009FFFF : Unused
	0x000A0000 - 0x000BFFFF : Video RAM video memory
	0x000B0000 - 0x000B7777 : Monochrome video memory (multiple pages)
	0x000B8000 - 0x000BFFFF : Color video memory (multiple pages)
	0x000C0000 - 0x000C7FFF : Video ROM video BIOS
	0x000C8000 - 0x000EFFFF : BIOS shadow area
	0x000F0000 - 0x000FFFFF : System BIOS
*/

/*
 * Important information:
 * 	- EFLAGS-register:		see intel manual, vol3a, page 66
 * 	- Control-Registers:	see intel manual, vol3a, page 71
 * 	- Segment-descriptors:	see intel manual, vol3a, page 101
 * 	- pt- and pd-entries:	see intel manual, vol3a, page 116
 * 	- pmode exceptions:		see intel manual, vol3a, page 191
 * 	- idt-descriptors:		see intel manual, vol3a, page 202
 */

static u8 initloader[] = {
	#include "../../build/user_initloader.dump"
};

s32 main(sMultiBoot *mbp,u32 magic) {
	u32 entryPoint;

	UNUSED(magic);

	/* the first thing we've to do is set up the page-dir and page-table for the kernel and so on
	 * and "correct" the GDT */
	paging_init();
	gdt_init();
	mboot_init(mbp);

	/* init video */
	vid_init();

	vid_printf("GDT exchanged, paging enabled, video initialized");
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

#if DEBUGGING
	mboot_dbg_print();
#endif

	/* mm */
	vid_printf("Initializing physical memory-management...");
	mm_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

	/* paging */
	vid_printf("Initializing paging...");
	paging_mapHigherHalf();
	paging_initCOWList();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

	/* fpu */
	vid_printf("Initializing FPU...");
	fpu_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

	/* vfs */
	vid_printf("Initializing VFS...");
	vfs_init();
	vfsinfo_init();
	vfsreq_init();
	vfsdrv_init();
	vfsr_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

	/* processes */
	vid_printf("Initializing process-management...");
	proc_init();
	sched_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

	/* idt */
	vid_printf("Initializing IDT...");
	intrpt_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

	/* timer */
	vid_printf("Initializing Timer...");
	timer_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

	/* signals */
	vid_printf("Initializing Signal-Handling...");
	sig_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

	/* kevents */
	vid_printf("Initializing KEvents...");
	kev_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

	/* cpu */
	vid_printf("Detecting CPU...");
	cpu_detect();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033[co;2]%s\033[co]","DONE");

#if DEBUGGING
	vid_printf("Free frames=%d, pages mapped=%d, free mem=%d KiB\n",
			mm_getFreeFrmCount(MM_DMA | MM_DEF),paging_dbg_getPageCount(),
			mm_getFreeFrmCount(MM_DMA | MM_DEF) * PAGE_SIZE / K);
#endif

	/* TODO: in guishell: $ cat /system/processes/23/virtmem --> page-fault */

#if 1
	/* load initloader */
	entryPoint = elf_loadFromMem(initloader,sizeof(initloader));
	/* give the process some stack pages */
	proc_changeSize(INITIAL_STACK_PAGES,CHG_STACK);
	return entryPoint;
#else
	while(1);
	return 0;
#endif
}
