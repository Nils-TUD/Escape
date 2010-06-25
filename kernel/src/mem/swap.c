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
#include <task/proc.h>
#include <task/thread.h>
#include <task/sched.h>
#include <mem/paging.h>
#include <mem/sharedmem.h>
#include <mem/kheap.h>
#include <mem/swap.h>
#include <mem/swapmap.h>
#include <video.h>
#include <string.h>
#include <assert.h>
#include <config.h>

#define SWAP_SIZE			(10 * M)
#define HIGH_WATER			40
#define LOW_WATER			20
#define CRIT_WATER			10

#define SW_TYPE_DEF			0
#define SW_TYPE_SHM			1
#define SW_TYPE_TEXT		2

#define MAX_SWAP_AT_ONCE	10

#define vid_printf(...)

static void swap_doSwapin(tTid tid,tFileNo file,sProc *p,u32 addr);
static void swap_doSwapOut(tTid tid,tFileNo file,sProc *p,u32 addr);
static sSLList *swap_getAffectedProcs(sProc *p,u32 addr,u8 *type);
static void swap_freeAffectedProcs(sSLList *procs,u8 type);
static u32 swap_getPageAddr(sProc *p,sProc *other,u32 addr,u8 type);
static void swap_setBlocked(sSLList *procs,bool blocked);
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
	const char *dev = NULL;/*conf_getStr(CONF_SWAP_DEVICE);*/
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
	sSLList *procs;
	u32 temp,frame;
	u8 type;
	u32 block;
	addr &= ~(PAGE_SIZE - 1);

	vid_printf("[%d] Swap %p of %s(%d) in ",t->tid,addr,p->command,p->pid);
	procs = swap_getAffectedProcs(p,addr,&type);

	/* we have to look through all processes because the one that swapped it out is not necessary
	 * the process that aquires it first again (text-sharing, ...) */
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *sp = (sProc*)n->data;
		block = swmap_find(sp->pid,swap_getPageAddr(p,sp,addr,type));
		if(block != INVALID_BLOCK) {
			vid_printf("(alloc by %d) ",sp->pid);
			swmap_free(sp->pid,block,1);
			break;
		}
	}
	if(block == INVALID_BLOCK) {
		/* this may actually happen if we want to swap a page of one process in but can't because
		 * we're swapping out for example. if another process that shares the page with the first
		 * one wants to swap this page in, too, it will wait as well. When we're done with
		 * swapping out, one of the processes swaps in. Then the second one wants to swap in
		 * the same page but doesn't find it in the swapmap since it has already been swapped in */
		vid_printf("Seems already do be swapped in -> skipping (%d @ %p)\n",p->pid,addr);
		swap_freeAffectedProcs(procs,type);
		return;
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

	/* mark page as 'not swapped' and 'present' and do the actual swapping */
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *sp = (sProc*)n->data;
		/* TODO */
		/*paging_swapIn(sp,swap_getPageAddr(p,sp,addr,type),frame);*/
		/* we've swapped in one page */
		sp->swapped--;
	}

	vid_printf("Done\n");
	swap_freeAffectedProcs(procs,type);
}

static void swap_doSwapOut(tTid tid,tFileNo file,sProc *p,u32 addr) {
	sSLList *procs;
	sSLNode *n;
	u8 type;
	u32 block;

	procs = swap_getAffectedProcs(p,addr,&type);
	block = swmap_alloc(p->pid,type == SW_TYPE_DEF ? NULL : procs,addr,1);
	vid_printf("[%d] Swap %p of %s(%d) out to blk %d...",tid,addr,p->command,p->pid,block);
	assert(block != INVALID_BLOCK);

	/* mark page as 'swapped out' and 'not-present' and do the actual swapping */
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *sp = (sProc*)n->data;
		/* TODO */
		u32 phys = 0;/*paging_swapOut(sp,swap_getPageAddr(p,sp,addr,type)) >> PAGE_SIZE_SHIFT;*/

		/* if the first one, do the actual swapping */
		if(n == sll_begin(procs)) {
			/* copy to a temporary buffer because we can't use the temp-area when switching
			 * threads */
			u32 temp = paging_mapToTemp(&phys,1);
			memcpy(buffer,(void*)temp,PAGE_SIZE);
			paging_unmapFromTemp(1);
			/* frame is no longer needed, so free it */
			mm_freeFrame(phys,MM_DEF);

			assert(vfs_seek(tid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
			assert(vfs_writeFile(tid,file,buffer,PAGE_SIZE) == PAGE_SIZE);
		}

		/* we've swapped one page */
		sp->swapped++;
	}

	vid_printf("Done\n");
	swap_freeAffectedProcs(procs,type);
}

static sSLList *swap_getAffectedProcs(sProc *p,u32 addr,u8 *type) {
	sSLList *procs;
	/* belongs to text? so we have to change the mapping for all users of the text */
	/* TODO */
	if(false/*addr < p->textPages * PAGE_SIZE*/) {
		/*vassert(p->text,"Process %d (%s) has textpages but no text!?",p->pid,p->command);
		procs = p->text->procs;*/
		*type = SW_TYPE_TEXT;
	}
	else {
		/* check whether it belongs to a shared-memory-region */
		/* TODO */
		procs = NULL/*shm_getMembers(p,addr)*/;
		if(!procs) {
			/* default case: create a linked list just with the process */
			procs = sll_create();
			if(procs == NULL)
				util_panic("Not enough kheap-mem");
			if(!sll_append(procs,p))
				util_panic("Not enough kheap-mem");
			*type = SW_TYPE_DEF;
		}
		else
			*type = SW_TYPE_SHM;
	}
	/* block all threads that are affected of the swap-operation */
	swap_setBlocked(procs,true);
	return procs;
}

static void swap_freeAffectedProcs(sSLList *procs,u8 type) {
	/* threads can continue now */
	swap_setBlocked(procs,false);
	/* if default, destroy the list */
	if(type == SW_TYPE_DEF)
		sll_destroy(procs,false);
}

static u32 swap_getPageAddr(sProc *p,sProc *other,u32 addr,u8 type) {
	if(type == SW_TYPE_SHM) {
		u32 otherAddr = shm_getAddrOfOther(p,addr,other);
		assert(otherAddr != 0);
		return otherAddr;
	}
	return addr;
}

static void swap_setBlocked(sSLList *procs,bool blocked) {
	sSLNode *n,*tn;
	sProc *p;
	sThread *t;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		p = (sProc*)n->data;
		for(tn = sll_begin(p->threads); tn != NULL; tn = tn->next) {
			t = (sThread*)tn->data;
			thread_setSuspended(t->tid,blocked);
		}
	}
}

static bool swap_findVictim(sProc **p,u32 *addr) {
	sProc *vp = proc_getLRUProc();
	if(vp == NULL)
		return false;
	*p = vp;
	/* TODO */
	*addr = 0;/*paging_swapGetNextAddr(vp);*/
	return true;
}
