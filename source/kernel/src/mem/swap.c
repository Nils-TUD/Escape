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
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/mem/swap.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/vmm.h>
#include <sys/mem/sllnodes.h>
#include <sys/mem/cache.h>
#include <sys/vfs/node.h>
#include <sys/video.h>
#include <sys/config.h>
#include <sys/klock.h>
#include <string.h>
#include <assert.h>

typedef struct {
	uintptr_t addr;
	sThread *thread;
} sSwapInJob;

/* TODO we can't swap out contiguous allocated frames, right? because we would need exactly this
 * frame again when swapping in. */

/* TODO we need a way to ask ata for a partition-size */
#define SWAP_SIZE			(10 * M)
#define MAX_SWAP_AT_ONCE	10

static bool enabled = false;
static bool swapping = false;
static sThread *swapper = NULL;
/* TODO calculate that from the available memory */
static size_t kframes = 1000;
static size_t uframes = 0;
static klock_t swapLock;
static sSLList swapInJobs;

void swap_start(void) {
	file_t swapFile = -1;
	sThread *t = thread_getRunning();
	inode_t swapIno;
	const char *dev = conf_getStr(CONF_SWAP_DEVICE);
	/* if there is no valid swap-dev specified, don't even try... */
	if(dev == NULL || vfs_node_resolvePath(dev,&swapIno,NULL,0) < 0) {
		while(1) {
			/* wait for ever */
			ev_block(t);
			thread_switchNoSigs();
		}
	}

	/* don't lock that, it might increase the kheap-size */
	swmap_init(SWAP_SIZE);
	klock_aquire(&swapLock);
	sll_init(&swapInJobs,slln_allocNode,slln_freeNode);
	swapper = t;
	swapFile = vfs_openPath(swapper->proc->pid,VFS_READ | VFS_WRITE,dev);
	assert(swapFile >= 0);
	enabled = true;

	/* start main-loop; wait for work */
	while(1) {
		size_t free = pmem_getFreeFrames(MM_DEF);
		/* swapping out is more important than swapping in */
		if((free - kframes) < uframes) {
			size_t amount = MIN(MAX_SWAP_AT_ONCE,uframes - (free - kframes));
			swapping = true;
			klock_release(&swapLock);

			vmm_swapOut(swapper->proc->pid,swapFile,amount);

			klock_aquire(&swapLock);
			swapping = false;
			ev_wakeup(EVI_SWAP_FREE,0);
		}

		/* handle swap-in-jobs */
		while(sll_length(&swapInJobs) > 0) {
			sSwapInJob *job = (sSwapInJob*)sll_removeFirst(&swapInJobs);
			swapping = true;
			klock_release(&swapLock);

			vmm_swapIn(swapper->proc->pid,swapFile,job->thread,job->addr);

			klock_aquire(&swapLock);
			ev_unblock(job->thread);
			swapping = false;
		}

		if(pmem_getFreeFrames(MM_DEF) - kframes >= uframes) {
			/* we may receive new work now */
			ev_wait(swapper,EVI_SWAP_WORK,0);
			klock_release(&swapLock);
			thread_switch();
			klock_aquire(&swapLock);
		}
	}
	vfs_closeFile(swapper->proc->pid,swapFile);
	klock_release(&swapLock);
}

void swap_reserve(size_t frameCount) {
	size_t free;
	sThread *t;
	klock_aquire(&swapLock);
	free = pmem_getFreeFrames(MM_DEF);
	uframes += frameCount;
	if(free >= frameCount && free - frameCount >= kframes) {
		klock_release(&swapLock);
		return;
	}

	/* swapping disabled or invalid thread? */
	t = thread_getRunning();
	if(!enabled || t->tid == ATA_TID || t->tid == swapper->tid)
		util_panic("Out of memory");

	do {
		/* notify swapper-thread */
		if(!swapping)
			ev_wakeupThread(swapper,EV_SWAP_WORK);
		ev_wait(t,EVI_SWAP_FREE,0);
		klock_release(&swapLock);
		thread_switchNoSigs();
		klock_aquire(&swapLock);
		free = pmem_getFreeFrames(MM_DEF);
	}
	while(free - kframes < frameCount);
	klock_release(&swapLock);
}

frameno_t swap_allocate(bool critical) {
	frameno_t frm = 0;
	klock_aquire(&swapLock);
	if(critical) {
		if(kframes == 0)
			util_panic("Not enough kernel-memory");
		frm = pmem_allocate();
		kframes--;
	}
	else {
		size_t free = pmem_getFreeFrames(MM_DEF);
		if(free > kframes) {
			assert(uframes > 0);
			frm = pmem_allocate();
			uframes--;
		}
	}
	klock_release(&swapLock);
	return frm;
}

void swap_free(frameno_t frame,bool critical) {
	klock_aquire(&swapLock);
	if(critical)
		kframes++;
	pmem_free(frame);
	klock_release(&swapLock);
}

bool swap_in(uintptr_t addr) {
	sThread *t;
	sSwapInJob *job;
	if(!enabled)
		return false;

	/* create and append a new swap-in-job */
	t = thread_getRunning();
	klock_aquire(&swapLock);
	job = cache_alloc(sizeof(sSwapInJob));
	if(!job) {
		klock_release(&swapLock);
		return false;
	}
	job->addr = addr;
	job->thread = t;
	if(!sll_append(&swapInJobs,job)) {
		cache_free(job);
		klock_release(&swapLock);
		return false;
	}
	/* notify the swapper, if necessary */
	if(!swapping)
		ev_wakeupThread(swapper,EV_SWAP_WORK);
	/* wait until its done */
	ev_block(t);
	klock_release(&swapLock);
	thread_switchNoSigs();
	cache_free(job);
	return true;
}
