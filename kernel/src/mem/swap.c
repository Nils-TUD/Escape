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

#define SWAP_SIZE		(10 * M)
#define HIGH_WATER		40
#define LOW_WATER		20
#define CRIT_WATER		10

static void swap_doSwapin(tTid tid,tFileNo file,sProc *p,u32 addr);
static void swap_doSwapOut(tTid tid,tFileNo file,sProc *p,u32 addr);
static sSLList *swap_getAffectedProcs(sProc *p,u32 addr,bool *isShm);
static void swap_freeAffectedProcs(sSLList *procs,sProc *p,u32 addr,bool isShm);
static void swap_setBlocked(sSLList *procs,bool blocked);
static bool swap_findVictim(sProc **p,u32 *addr);

static bool enabled = false;
static bool swapping = false;
static sProc *swapinProc = NULL;
static u32 swapinAddr = 0;
static tTid swapinTid = INVALID_TID;
static tTid swapperTid = INVALID_TID;

/* TODO remove the stuff from the swapmap if a process terminates */
/* TODO we have a problem if one process tries to swap too much in without giving
 * others the chance to swap something out */
/* TODO we have problems with shared memory. like text-sharing we have to check which
 * processes use it and give all the same "last-usage-time". otherwise we have trashing... */
/* TODO additionally, shared mem is marked as "no-free" in all joined processes. is this a problem? */

/* TODO why do the processes (not ata and init!) do sooo much time in userspace while swapping? */

/* concept:
 *
 * - swapmap stores a list of processes that use the memory (not an own list, but the list
 * 		from text-sharing or shared-memory)
 * - swmap_remProc() will be called BEFORE removing the process from shmem / textsharing. This way
 * 		the list remains valid. If just one process is in the list and thats the process to remove,
 * 		we can remove it from swap. Otherwise its either not our memory or we're not the last one.
 * - shared memory can't use the nofree flag since paging needs this as indicator to decide wether
 * 		a page can be swapped or not. So we need a different way to keep track of the shared-memory
 * 		regions. The nofree-flag was just used for removing on process-destroy anyway, so thats
 * 		the only part we've to change.
 * - shmem will not just store one entry per shmem-area but one for each usage and store wether
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
		if(mm_getFreeFrmCount(MM_DEF) < LOW_WATER) {
			swapping = true;
			while(mm_getFreeFrmCount(MM_DEF) < HIGH_WATER) {
				sProc *p;
				u32 addr;
				assert(swap_findVictim(&p,&addr));
				swap_doSwapOut(swapperTid,swapFile,p,addr);
			}
			swapping = false;
		}
		if(swapinProc != NULL) {
			swapping = true;
			swap_doSwapin(swapperTid,swapFile,swapinProc,swapinAddr);
			thread_wakeup(swapinTid,EV_SWAP_DONE);
			swapinProc = NULL;
			swapping = false;
		}

		/* we may receive new work now */
		thread_wakeupAll(0,EV_SWAP_FREE);
		thread_wait(swapperTid,0,EV_SWAP_WORK);
		thread_switch();
	}
	vfs_closeFile(swapperTid,swapFile);
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
	u32 frame;
	bool isShm = false;
	u8 *buffer;
	u32 block;
	addr &= ~(PAGE_SIZE - 1);

	vid_printf("[%d] Swap %p of %s(%d) in ",t->tid,addr,p->command,p->pid);
	procs = swap_getAffectedProcs(p,addr,&isShm);

	/* we have to look through all processes because the one that swapped it out is not necessary
	 * the process that aquires it first again (text-sharing, ...) */
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *sp = (sProc*)n->data;
		block = swmap_find(sp->pid,addr);
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
		swap_freeAffectedProcs(procs,p,addr,isShm);
		return;
	}

	vid_printf("from blk %d...",block);

	buffer = kheap_alloc(PAGE_SIZE);
	assert(buffer != NULL);

	/* read into buffer */
	assert(vfs_seek(tid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
	assert(vfs_readFile(tid,file,buffer,PAGE_SIZE) == PAGE_SIZE);

	/* copy into a new frame */
	assert(mm_getFreeFrmCount(MM_DEF) > 0);
	frame = mm_allocateFrame(MM_DEF);
	paging_map(TEMP_MAP_AREA,&frame,1,PG_SUPERVISOR,true);
	memcpy((void*)TEMP_MAP_AREA,buffer,PAGE_SIZE);
	paging_unmap(TEMP_MAP_AREA,1,false,false);
	kheap_free(buffer);

	/* mark page as 'not swapped' and 'present' and do the actual swapping */
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *sp = (sProc*)n->data;
		paging_swapIn(sp,addr,frame);
		/* we've swapped in one page */
		sp->swapped--;
	}

	vid_printf("Done\n");
	swap_freeAffectedProcs(procs,p,addr,isShm);
}

static void swap_doSwapOut(tTid tid,tFileNo file,sProc *p,u32 addr) {
	sSLList *procs;
	sSLNode *n;
	bool isShm = false;
	u32 block = swmap_alloc(p->pid,addr,1);
	assert(block != INVALID_BLOCK);

	vid_printf("[%d] Swap %p of %s(%d) out to blk %d...",tid,addr,p->command,p->pid,block);
	procs = swap_getAffectedProcs(p,addr,&isShm);

	/* mark page as 'swapped out' and 'not-present' and do the actual swapping */
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *sp = (sProc*)n->data;
		u32 phys = paging_swapOut(sp,addr);
		/* if the first one, do the actual swapping */
		if(n == sll_begin(procs)) {
			/* copy to a temporary buffer because we can't use TEMP_MAP_AREA when switching
			 * threads */
			u8 *buffer = kheap_alloc(PAGE_SIZE);
			assert(buffer != NULL);
			paging_map(TEMP_MAP_AREA,&phys,1,PG_SUPERVISOR | PG_ADDR_TO_FRAME,true);
			memcpy(buffer,(void*)TEMP_MAP_AREA,PAGE_SIZE);
			paging_unmap(TEMP_MAP_AREA,1,false,false);
			/* frame is no longer needed, so free it */
			mm_freeFrame(phys >> PAGE_SIZE_SHIFT,MM_DEF);

			assert(vfs_seek(tid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
			assert(vfs_writeFile(tid,file,buffer,PAGE_SIZE) == PAGE_SIZE);
			kheap_free(buffer);
		}

		/* we've swapped one page */
		sp->swapped++;
	}

	vid_printf("Done\n");
	swap_freeAffectedProcs(procs,p,addr,isShm);
}

static sSLList *swap_getAffectedProcs(sProc *p,u32 addr,bool *isShm) {
	sSLList *procs;
	/* belongs to text? so we have to change the mapping for all users of the text */
	if(addr < p->textPages * PAGE_SIZE)
		procs = p->text->procs;
	else {
		/* check wether it belongs to a shared-memory-region. this is just the case if we're the
		 * owner because for the members it is mapped as "no-free" so we won't get the page anyway */
		procs = shm_getMembers(p,addr);
		if(!procs) {
			/* default case: create a linked list just with the process */
			procs = sll_create();
			assert(procs != NULL);
		}
		else
			*isShm = true;
		/* Note: the owner of a shared-memory-region is no member. therefore we add the owner
		 * here temporary and remove him later */
		assert(sll_append(procs,p));
	}
	/* block all threads that are affected of the swap-operation */
	swap_setBlocked(procs,true);
	return procs;
}

static void swap_freeAffectedProcs(sSLList *procs,sProc *p,u32 addr,bool isShm) {
	/* threads can continue now */
	swap_setBlocked(procs,false);
	/* if it is shared mem remove the process */
	if(isShm)
		sll_removeFirst(procs,p);
	/* if no text and no shm, destroy the list */
	else if(addr >= p->textPages * PAGE_SIZE)
		sll_destroy(procs,false);
}

static void swap_setBlocked(sSLList *procs,bool blocked) {
	sSLNode *n,*tn;
	sProc *p;
	sThread *t;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		p = (sProc*)n->data;
		for(tn = sll_begin(p->threads); tn != NULL; tn = tn->next) {
			t = (sThread*)tn->data;
			sched_setBlockForSwap(t,blocked);
		}
	}
}

static bool swap_findVictim(sProc **p,u32 *addr) {
	sProc *vp = proc_getLRUProc();
	if(vp == NULL)
		return false;
	*p = vp;
	*addr = paging_swapGetNextAddr(vp);
	return true;
}
