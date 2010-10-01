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
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/sched.h>
#include <sys/mem/paging.h>
#include <sys/mem/sharedmem.h>
#include <sys/mem/kheap.h>
#include <sys/mem/swap.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/vmm.h>
#include <sys/video.h>
#include <sys/config.h>
#include <string.h>
#include <assert.h>

/* TODO we need a way to ask ata for a partition-size */
#define SWAP_SIZE			(10 * M)
#define HIGH_WATER			70
#define LOW_WATER			50
#define CRIT_WATER			20

#define MAX_SWAP_AT_ONCE	10

#define vid_printf(...)

static void swap_doSwapin(tPid pid,tFileNo file,sProc *p,u32 addr);
static void swap_doSwapOut(tPid pid,tFileNo file,sRegion *reg,u32 index);
static void swap_setSuspended(sSLList *procs,bool blocked);
static sRegion *swap_findVictim(u32 *index);

static bool enabled = false;
static bool swapping = false;
static sProc *swapinProc = NULL;
static u32 swapinAddr = 0;
static tTid swapinTid = INVALID_TID;
static sThread *swapper = NULL;
static u32 neededFrames = HIGH_WATER;
/* no heap-usage here */
static u8 buffer[PAGE_SIZE];

void swap_start(void) {
	tFileNo swapFile = -1;
	sThread *t = thread_getRunning();
	tInodeNo swapIno;
	const char *dev = conf_getStr(CONF_SWAP_DEVICE);
	/* if there is no valid swap-dev specified, don't even try... */
	if(dev == NULL || vfsn_resolvePath(dev,&swapIno,NULL,0) < 0) {
		while(1) {
			/* wait for ever */
			thread_wait(t->tid,NULL,EV_NOEVENT);
			thread_switchNoSigs();
		}
	}

	swmap_init(SWAP_SIZE);
	swapper = t;
	swapFile = vfs_openPath(swapper->proc->pid,VFS_READ | VFS_WRITE,dev);
	assert(swapFile >= 0);
	enabled = true;

	/* start main-loop; wait for work */
	while(1) {
		/* swapping out is more important than swapping in to prevent that we run out of memory */
		if(mm_getFreeFrames(MM_DEF) < LOW_WATER || neededFrames > HIGH_WATER) {
			vid_printf("Starting to swap out (%d free frames; %d needed)\n",
					mm_getFreeFrames(MM_DEF),neededFrames);
			u32 count = 0;
			swapping = true;
			while(count < MAX_SWAP_AT_ONCE && mm_getFreeFrames(MM_DEF) < neededFrames) {
				u32 index,free;
				sRegion *reg = swap_findVictim(&index);
				if(reg == NULL)
					util_panic("No process to swap out");
				swap_doSwapOut(swapper->proc->pid,swapFile,reg,index);
				/* notify the threads that require the currently available frame-count */
				free = mm_getFreeFrames(MM_DEF);
				if(free > HIGH_WATER)
					thread_wakeupAll((void*)(free - HIGH_WATER),EV_SWAP_FREE);
				count++;
			}
			/* if we've reached the needed frame-count, reset it */
			if(mm_getFreeFrames(MM_DEF) >= neededFrames)
				neededFrames = HIGH_WATER;
			swapping = false;
		}
		if(swapinProc != NULL) {
			swapping = true;
			swap_doSwapin(swapper->proc->pid,swapFile,swapinProc,swapinAddr);
			thread_wakeup(swapinTid,EV_SWAP_DONE);
			swapinProc = NULL;
			swapping = false;
		}

		if(mm_getFreeFrames(MM_DEF) >= LOW_WATER && neededFrames == HIGH_WATER) {
			/* we may receive new work now */
			thread_wakeupAll(NULL,EV_SWAP_FREE);
			thread_wait(swapper->tid,NULL,EV_SWAP_WORK);
			thread_switch();
		}
	}
	vfs_closeFile(swapper->proc->pid,swapFile);
}

bool swap_outUntil(u32 frameCount) {
	u32 free = mm_getFreeFrames(MM_DEF);
	sThread *t = thread_getRunning();
	if(free >= frameCount)
		return true;
	if(!enabled || t->tid != IDLE_TID || t->tid == ATA_TID || t->tid == swapper->tid)
		return false;
	do {
		/* notify swapper-thread */
		if(!swapping)
			thread_wakeup(swapper->tid,EV_SWAP_WORK);
		neededFrames += frameCount - free;
		thread_wait(t->tid,(void*)(frameCount - free),EV_SWAP_FREE);
		thread_switchNoSigs();
		/* TODO report error if swap-space is full or nothing left to swap */
		free = mm_getFreeFrames(MM_DEF);
	}
	while(free < frameCount);
	return true;
}

void swap_check(void) {
	u32 freeFrm;
	if(!enabled)
		return;

	freeFrm = mm_getFreeFrames(MM_DEF);
	if(freeFrm < LOW_WATER/* || neededFrames < HIGH_WATER*/) {
		/* notify swapper-thread */
		if(/*freeFrm < LOW_WATER && */!swapping)
			thread_wakeup(swapper->tid,EV_SWAP_WORK);
		/* if we have VERY few frames left, better block this thread until we have high water */
		if(freeFrm < CRIT_WATER/* || neededFrames < HIGH_WATER*/) {
			sThread *t = thread_getRunning();
			/* but its not really helpful to block ata ;) */
			if(t->tid != ATA_TID && t->tid != IDLE_TID) {
				thread_wait(t->tid,NULL,EV_SWAP_FREE);
				thread_switchNoSigs();
			}
		}
	}
}

bool swap_in(sProc *p,u32 addr) {
	sThread *t = thread_getRunning();
	if(!enabled)
		return false;
	/* wait here until we're alone */
	while(swapping || swapinProc) {
		thread_wait(t->tid,NULL,EV_SWAP_FREE);
		thread_switchNoSigs();
	}

	/* give the swapper work */
	swapinProc = p;
	swapinAddr = addr;
	swapinTid = t->tid;
	thread_wait(t->tid,NULL,EV_SWAP_DONE);
	thread_wakeup(swapper->tid,EV_SWAP_WORK);
	thread_switchNoSigs();
	return true;
}

static void swap_doSwapin(tPid pid,tFileNo file,sProc *p,u32 addr) {
	tVMRegNo rno = vmm_getRegionOf(p,addr);
	sVMRegion *vmreg = vmm_getRegion(p,rno);
	u32 temp,frame,block,index;
	addr &= ~(PAGE_SIZE - 1);

	index = (addr - vmreg->virt) / PAGE_SIZE;
	swap_setSuspended(vmreg->reg->procs,true);

	/* not swapped anymore? so probably another process has already swapped it in */
	/* this may actually happen if we want to swap a page of one process in but can't because
	 * we're swapping out for example. if another process that shares the page with the first
	 * one wants to swap this page in, too, it will wait as well. When we're done with
	 * swapping out, one of the processes swaps in. Then the second one wants to swap in
	 * the same page but doesn't find it in the swapmap since it has already been swapped in */
	if(!(vmreg->reg->pageFlags[index] & PF_SWAPPED)) {
		swap_setSuspended(vmreg->reg->procs,false);
		return;
	}

	block = reg_getSwapBlock(vmreg->reg,index);

	vid_printf("Swapin %x:%d (first=%p %s:%d) from blk %d...",
			vmreg->reg,index,addr,p->command,p->pid,block);

	/* read into buffer */
	assert(vfs_seek(pid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
	assert(vfs_readFile(pid,file,buffer,PAGE_SIZE) == PAGE_SIZE);

	/* copy into a new frame */
	if(mm_getFreeFrames(MM_DEF) == 0)
		util_panic("No free frame to swap in");
	frame = mm_allocate();
	temp = paging_mapToTemp(&frame,1);
	memcpy((void*)temp,buffer,PAGE_SIZE);
	paging_unmapFromTemp(1);

	/* mark as not-swapped and map into all affected processes */
	vmm_swapIn(vmreg->reg,index,frame);
	/* free swap-block */
	swmap_free(block);

	vid_printf("Done\n");
	swap_setSuspended(vmreg->reg->procs,false);
}

static void swap_doSwapOut(tPid pid,tFileNo file,sRegion *reg,u32 index) {
	tVMRegNo rno;
	sVMRegion *vmreg;
	u32 block,frameNo,temp;
	sProc *first = (sProc*)sll_get(reg->procs,0);

	swap_setSuspended(reg->procs,true);
	rno = vmm_getRNoByRegion(first,reg);
	vmreg = vmm_getRegion(first,rno);
	block = swmap_alloc();
	vid_printf("Swapout free=%d %x:%d (first=%p %s:%d) to blk %d...",
			mm_getFreeFrames(MM_DEF),reg,index,vmreg->virt + index * PAGE_SIZE,
			first->command,first->pid,block);
	assert(block != INVALID_BLOCK);

	/* copy to a temporary buffer because we can't use the temp-area when switching threads */
	frameNo = paging_getFrameNo(first->pagedir,vmreg->virt + index * PAGE_SIZE);
	temp = paging_mapToTemp(&frameNo,1);
	memcpy(buffer,(void*)temp,PAGE_SIZE);
	paging_unmapFromTemp(1);

	/* mark as swapped and unmap from processes */
	reg_setSwapBlock(reg,index,block);
	vmm_swapOut(reg,index);
	mm_free(frameNo);

	/* write out on disk */
	assert(vfs_seek(pid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
	assert(vfs_writeFile(pid,file,buffer,PAGE_SIZE) == PAGE_SIZE);

	vid_printf("Done\n");
	swap_setSuspended(reg->procs,false);
}

static void swap_setSuspended(sSLList *procs,bool blocked) {
	sSLNode *n,*tn;
	sProc *p;
	sThread *cur = thread_getRunning();
	sThread *t;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		p = (sProc*)n->data;
		for(tn = sll_begin(p->threads); tn != NULL; tn = tn->next) {
			t = (sThread*)tn->data;
			if(t->tid != cur->tid)
				thread_setSuspended(t->tid,blocked);
		}
	}
}

static sRegion *swap_findVictim(u32 *index) {
	sRegion *reg = proc_getLRURegion();
	if(reg == NULL)
		return NULL;
	*index = vmm_getPgIdxForSwap(reg);
	return reg;
}
