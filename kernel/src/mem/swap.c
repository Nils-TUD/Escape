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

#define SWAP_SIZE			(10 * M)
#define HIGH_WATER			40
#define LOW_WATER			20
#define CRIT_WATER			10

#define SW_TYPE_DEF			0
#define SW_TYPE_SHM			1
#define SW_TYPE_TEXT		2

#define MAX_SWAP_AT_ONCE	10

/*#define vid_printf(...)*/

static void swap_doSwapin(tTid tid,tFileNo file,sProc *p,u32 addr);
static void swap_doSwapOut(tTid tid,tFileNo file,sProc *p,u32 addr);
static sSLList *swap_getAffectedProcs(sProc *p,tVMRegNo *rno,u32 addr);
static void swap_freeAffectedProcs(sSLList *procs);
static void swap_setSuspended(sSLList *procs,bool blocked);
static bool swap_findVictim(sProc **p,u32 *addr);

static bool enabled = false;
static bool swapping = false;
static sProc *swapinProc = NULL;
static u32 swapinAddr = 0;
static tTid swapinTid = INVALID_TID;
static tTid swapperTid = INVALID_TID;
static u32 neededFrames = HIGH_WATER;
/* no heap-usage here */
static u8 buffer[PAGE_SIZE];

/* TODO we have problems with shared memory. like text-sharing we have to check which
 * processes use it and give all the same "last-usage-time". otherwise we have trashing... */
/* TODO additionally copy-on-write is not swapped */
/* TODO a problem is also that we can't yet clone processes that have swapped something out, right? */

/* concept:
 *
 * - swapmap stores a list of processes that use the memory (not an own list, but the list
 * 		from text-sharing or shared-memory)
 * - swmap_remProc() will be called BEFORE removing the process from shmem / textsharing. This way
 * 		the list remains valid. If just one process is in the list and thats the process to remove,
 * 		we can remove it from swap. Otherwise its either not our memory or we're not the last one.
 * - shared memory can't use the nofree flag since paging needs this as indicator to decide whether
 * 		a page can be swapped or not. So we need a different way to keep track of the shared-memory
 * 		regions. The nofree-flag was just used for removing on process-destroy anyway, so thats
 * 		the only part we've to change.
 * - shmem will not just store one entry per shmem-area but one for each usage and store whether
 * 		its the owner or not. This way we can save the location of it in virtual memory for all
 * 		users.
 * - additionally shmem will put the owner into the memberlist, too. That makes it easier.
 */

void swap_start(void) {
	tFileNo swapFile = -1;
	sThread *t = thread_getRunning();
	tInodeNo swapIno;
	const char *dev = conf_getStr(CONF_SWAP_DEVICE);
	/* if there is no valid swap-dev specified, don't even try... */
	if(dev == NULL || vfsn_resolvePath(dev,&swapIno,NULL,VFS_CONNECT) < 0) {
		while(1) {
			/* wait for ever */
			thread_wait(t->tid,0,EV_NOEVENT);
			thread_switchNoSigs();
		}
	}

	swmap_init(SWAP_SIZE);
	swapperTid = t->tid;
	swapFile = vfs_openFile(swapperTid,VFS_READ | VFS_WRITE,swapIno,VFS_DEV_NO);
	assert(swapFile >= 0);
	enabled = true;

	/* start main-loop; wait for work */
	while(1) {
		/* swapping out is more important than swapping in to prevent that we run out of memory */
		if(mm_getFreeFrmCount(MM_DEF) < LOW_WATER || neededFrames > HIGH_WATER) {
			u32 count = 0;
			swapping = true;
			while(count < MAX_SWAP_AT_ONCE && mm_getFreeFrmCount(MM_DEF) < neededFrames) {
				sProc *p;
				u32 addr,free;
				if(!swap_findVictim(&p,&addr))
					util_panic("No process to swap out");
				swap_doSwapOut(swapperTid,swapFile,p,addr);
				/* notify the threads that require the currently available frame-count */
				free = mm_getFreeFrmCount(MM_DEF);
				if(free > HIGH_WATER)
					thread_wakeupAll((void*)(free - HIGH_WATER),EV_SWAP_FREE);
				count++;
			}
			/* if we've reached the needed frame-count, reset it */
			if(mm_getFreeFrmCount(MM_DEF) >= neededFrames)
				neededFrames = HIGH_WATER;
			swapping = false;
		}
		if(swapinProc != NULL) {
			swapping = true;
			swap_doSwapin(swapperTid,swapFile,swapinProc,swapinAddr);
			thread_wakeup(swapinTid,EV_SWAP_DONE);
			swapinProc = NULL;
			swapping = false;
		}

		if(mm_getFreeFrmCount(MM_DEF) >= LOW_WATER && neededFrames == HIGH_WATER) {
			/* we may receive new work now */
			thread_wakeupAll(0,EV_SWAP_FREE);
			thread_wait(swapperTid,0,EV_SWAP_WORK);
			thread_switch();
		}
	}
	vfs_closeFile(swapperTid,swapFile);
}

bool swap_outUntil(u32 frameCount) {
	u32 free = mm_getFreeFrmCount(MM_DEF);
	sThread *t = thread_getRunning();
	if(free >= frameCount)
		return true;
	if(t->tid == ATA_TID || t->tid == swapperTid)
		return false;
	do {
		/* notify swapper-thread */
		if(!swapping)
			thread_wakeup(swapperTid,EV_SWAP_WORK);
		neededFrames += frameCount - free;
		thread_wait(t->tid,(void*)(frameCount - free),EV_SWAP_FREE);
		thread_switchNoSigs();
		/* TODO report error if swap-space is full or nothing left to swap */
		free = mm_getFreeFrmCount(MM_DEF);
	}
	while(free < frameCount);
	return true;
}

void swap_check(void) {
	u32 freeFrm;
	if(!enabled)
		return;

	freeFrm = mm_getFreeFrmCount(MM_DEF);
	if(freeFrm < LOW_WATER) {
		/* notify swapper-thread */
		if(!swapping)
			thread_wakeup(swapperTid,EV_SWAP_WORK);
		/* if we have VERY few frames left, better block this thread until we have high water */
		if(freeFrm < CRIT_WATER) {
			sThread *t = thread_getRunning();
			/* but its not really helpfull to block ata ;) */
			if(t->tid != ATA_TID) {
				thread_wait(t->tid,0,EV_SWAP_FREE);
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
		thread_wait(t->tid,0,EV_SWAP_FREE);
		thread_switchNoSigs();
	}

	/* give the swapper work */
	swapinProc = p;
	swapinAddr = addr;
	swapinTid = t->tid;
	thread_wait(t->tid,0,EV_SWAP_DONE);
	thread_wakeup(swapperTid,EV_SWAP_WORK);
	thread_switchNoSigs();
	return true;
}

static void swap_doSwapin(tTid tid,tFileNo file,sProc *p,u32 addr) {
	sThread *t = thread_getRunning();
	sSLNode *n;
	tVMRegNo rno;
	sVMRegion *vmreg;
	sSLList *procs;
	u32 temp,frame,block;
	addr &= ~(PAGE_SIZE - 1);

	procs = swap_getAffectedProcs(p,&rno,addr);
	vmreg = vmm_getRegion(p,rno);

	/* not swapped anymore? so probably another process has already swapped it in */
	/* this may actually happen if we want to swap a page of one process in but can't because
	 * we're swapping out for example. if another process that shares the page with the first
	 * one wants to swap this page in, too, it will wait as well. When we're done with
	 * swapping out, one of the processes swaps in. Then the second one wants to swap in
	 * the same page but doesn't find it in the swapmap since it has already been swapped in */
	if(!(vmreg->reg->pageFlags[(addr - vmreg->virt) / PAGE_SIZE] & PF_SWAPPED)) {
		swap_freeAffectedProcs(procs);
		return;
	}

	vid_printf("[%d] Swapin %p of %s(%d) ",t->tid,addr,p->command,p->pid);

	/* we have to look through all processes because the one that swapped it out is not necessary
	 * the process that aquires it first again (region-sharing) */
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *sp = (sProc*)n->data;
		if(sp != p) {
			tVMRegNo srno = vmm_getRNoByRegion(sp,vmreg->reg);
			sVMRegion *sreg = vmm_getRegion(sp,srno);
			block = swmap_find(sp->pid,sreg->virt + (addr - vmreg->virt));
		}
		else
			block = swmap_find(sp->pid,addr);
		if(block != INVALID_BLOCK) {
			vid_printf("(alloc by %d) ",sp->pid);
			swmap_free(sp->pid,block,1);
			break;
		}
	}
	if(block == INVALID_BLOCK) {
		swmap_dbg_print();
		util_panic("Page still swapped out but not found in swapmap!?");
	}

	vid_printf("from blk %d...",block);

	/* read into buffer */
	assert(vfs_seek(tid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
	assert(vfs_readFile(tid,file,buffer,PAGE_SIZE) == PAGE_SIZE);

	/* copy into a new frame */
	if(mm_getFreeFrmCount(MM_DEF) == 0)
		util_panic("No free frame to swap in");
	frame = mm_allocateFrame(MM_DEF);
	temp = paging_mapToTemp(&frame,1);
	memcpy((void*)temp,buffer,PAGE_SIZE);
	paging_unmapFromTemp(1);

	/* mark as not-swapped and map into all affected processes */
	vmm_swapIn(p,rno,addr,frame);

	vid_printf("Done\n");
	swap_freeAffectedProcs(procs);
}

static void swap_doSwapOut(tTid tid,tFileNo file,sProc *p,u32 addr) {
	sSLList *procs;
	tVMRegNo rno;
	u32 block,frameNo,temp;

	procs = swap_getAffectedProcs(p,&rno,addr);
	block = swmap_alloc(p->pid,procs,addr,1);
	vid_printf("[%d] Swapout %p of %s(%d) to blk %d...",tid,addr,p->command,p->pid,block);
	assert(block != INVALID_BLOCK);

	/* copy to a temporary buffer because we can't use the temp-area when switching threads */
	frameNo = paging_getFrameNo(p->pagedir,addr);
	temp = paging_mapToTemp(&frameNo,1);
	memcpy(buffer,(void*)temp,PAGE_SIZE);
	paging_unmapFromTemp(1);

	/* mark as swapped and unmap from processes */
	vmm_swapOut(p,rno,addr);

	/* write out on disk */
	assert(vfs_seek(tid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
	assert(vfs_writeFile(tid,file,buffer,PAGE_SIZE) == PAGE_SIZE);

	vid_printf("Done\n");
	swap_freeAffectedProcs(procs);
}

static sSLList *swap_getAffectedProcs(sProc *p,tVMRegNo *rno,u32 addr) {
	sSLList *procs;
	*rno = vmm_getRegionOf(p,addr);
	procs = vmm_getUsersOf(p,*rno);
	/* TODO handle copy-on-write */
	assert(procs);
	/* block all threads that are affected of the swap-operation */
	swap_setSuspended(procs,true);
	return procs;
}

static void swap_freeAffectedProcs(sSLList *procs) {
	/* threads can continue now */
	swap_setSuspended(procs,false);
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

static bool swap_findVictim(sProc **p,u32 *addr) {
	sProc **procs = NULL;
	u32 *pages = NULL;
	u32 proci,pagei,count = proc_getProcsForSwap(&procs,&pages);
	if(!count)
		return false;
	proci = util_rand() % count;
	pagei = util_rand() % pages[proci];
	*addr = vmm_getAddrForSwap(procs[proci],pagei);
	if(!*addr) {
		kheap_free(procs);
		kheap_free(pages);
		return false;
	}
	*p = procs[proci];
	kheap_free(procs);
	kheap_free(pages);
	return true;
}
