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
#include <sys/log.h>
#include <sys/video.h>
#include <sys/config.h>
#include <sys/spinlock.h>
#include <esc/messages.h>
#include <string.h>
#include <assert.h>

typedef struct {
	uintptr_t addr;
	sThread *thread;
} sSwapInJob;

/* TODO we can't swap out contiguous allocated frames, right? because we would need exactly this
 * frame again when swapping in. */
/* TODO should we lock pmem at all? if its just used by us, its not necessary */

#define KERNEL_MEM_PERCENT	20
#define KERNEL_MEM_MIN		750
#define MAX_SWAP_AT_ONCE	10

static size_t swappedOut = 0;
static size_t swappedIn = 0;
static bool enabled = false;
static bool swapping = false;
static sThread *swapper = NULL;
static size_t kframes = 0;
static size_t uframes = 0;
static klock_t swapLock;
static sSLList swapInJobs;

void swap_init(void) {
	size_t free;
	/* test whether a swap-device is present */
	if(conf_getStr(CONF_SWAP_DEVICE) != NULL) {
		/* init */
		sll_init(&swapInJobs,slln_allocNode,slln_freeNode);

		/* determine kernel-memory-size */
		free = pmem_getFreeFrames(MM_DEF);
		kframes = free / (100 / KERNEL_MEM_PERCENT);
		if(kframes < KERNEL_MEM_MIN)
			kframes = KERNEL_MEM_MIN;
		if(kframes > free / 2) {
			log_printf("Warning: Detected VERY small number of free frames\n");
			log_printf("         (%zu total, using %zu for kernel)\n",free,kframes);
		}
		enabled = true;
	}
}

bool swap_isEnabled(void) {
	return enabled;
}

void swap_start(void) {
	size_t free;
	file_t swapFile;
	const char *dev = conf_getStr(CONF_SWAP_DEVICE);
	swapper = thread_getRunning();
	assert(enabled);

	/* open device */
	swapFile = vfs_openPath(swapper->proc->pid,VFS_READ | VFS_WRITE | VFS_MSGS,dev);
	if(swapFile < 0) {
		log_printf("Unable to open swap-device '%s'\n",dev);
		enabled = false;
	}
	else {
		/* get device-size and init swap-map */
		sArgsMsg msg;
		assert(vfs_sendMsg(swapper->proc->pid,swapFile,MSG_DISK_GETSIZE,NULL,0,NULL,0,NULL,0) >= 0);
		assert(vfs_receiveMsg(swapper->proc->pid,swapFile,NULL,&msg,sizeof(msg)) >= 0);
		swmap_init(msg.arg1);
	}

	/* start main-loop; wait for work */
	spinlock_aquire(&swapLock);
	while(1) {
		free = pmem_getFreeFrames(MM_DEF);
		/* swapping out is more important than swapping in */
		if((free - kframes) < uframes) {
			size_t amount = MIN(MAX_SWAP_AT_ONCE,uframes - (free - kframes));
			swapping = true;
			spinlock_release(&swapLock);

			vmm_swapOut(swapper->proc->pid,swapFile,amount);
			swappedOut += amount;

			spinlock_aquire(&swapLock);
			swapping = false;
			ev_wakeup(EVI_SWAP_FREE,0);
		}

		/* handle swap-in-jobs */
		while(sll_length(&swapInJobs) > 0) {
			sSwapInJob *job = (sSwapInJob*)sll_removeFirst(&swapInJobs);
			swapping = true;
			spinlock_release(&swapLock);

			if(vmm_swapIn(swapper->proc->pid,swapFile,job->thread,job->addr))
				swappedIn++;

			spinlock_aquire(&swapLock);
			ev_unblock(job->thread);
			swapping = false;
		}

		if(pmem_getFreeFrames(MM_DEF) - kframes >= uframes) {
			/* we may receive new work now */
			ev_wait(swapper,EVI_SWAP_WORK,0);
			spinlock_release(&swapLock);
			thread_switch();
			spinlock_aquire(&swapLock);
		}
	}
	spinlock_release(&swapLock);
}

void swap_reserve(size_t frameCount) {
	size_t free;
	sThread *t;
	spinlock_aquire(&swapLock);
	free = pmem_getFreeFrames(MM_DEF);
	uframes += frameCount;
	/* enough user-memory available? */
	if(free >= frameCount && free - frameCount >= kframes) {
		spinlock_release(&swapLock);
		return;
	}

	/* swapping not possible? */
	t = thread_getRunning();
	if(!enabled || t->tid == ATA_TID || t->tid == swapper->tid)
		util_panic("Out of memory");

	/* wait until we have <frameCount> frames free */
	do {
		/* notify swapper-thread */
		if(!swapping)
			ev_wakeupThread(swapper,EV_SWAP_WORK);
		ev_wait(t,EVI_SWAP_FREE,0);
		spinlock_release(&swapLock);
		thread_switchNoSigs();
		spinlock_aquire(&swapLock);
		free = pmem_getFreeFrames(MM_DEF);
	}
	while(free - kframes < frameCount);
	spinlock_release(&swapLock);
}

frameno_t swap_allocate(bool critical) {
	frameno_t frm = 0;
	spinlock_aquire(&swapLock);
	if(enabled && critical) {
		if(kframes == 0)
			util_panic("Not enough kernel-memory");
		frm = pmem_allocate();
		kframes--;
	}
	else {
		size_t free = pmem_getFreeFrames(MM_DEF);
		if(free > kframes) {
			assert(critical || uframes > 0);
			frm = pmem_allocate();
			if(!critical)
				uframes--;
		}
	}
	spinlock_release(&swapLock);
	return frm;
}

void swap_free(frameno_t frame,bool critical) {
	spinlock_aquire(&swapLock);
	if(enabled && critical)
		kframes++;
	pmem_free(frame);
	spinlock_release(&swapLock);
}

bool swap_in(uintptr_t addr) {
	sThread *t;
	sSwapInJob *job;
	if(!enabled)
		return false;

	/* create and append a new swap-in-job */
	t = thread_getRunning();
	spinlock_aquire(&swapLock);
	job = cache_alloc(sizeof(sSwapInJob));
	if(!job) {
		spinlock_release(&swapLock);
		return false;
	}
	job->addr = addr;
	job->thread = t;
	if(!sll_append(&swapInJobs,job)) {
		cache_free(job);
		spinlock_release(&swapLock);
		return false;
	}
	/* notify the swapper, if necessary */
	if(!swapping)
		ev_wakeupThread(swapper,EV_SWAP_WORK);
	/* wait until its done */
	ev_block(t);
	spinlock_release(&swapLock);
	thread_switchNoSigs();
	cache_free(job);
	return true;
}

void swap_print(void) {
	sSLNode *n;
	const char *dev = conf_getStr(CONF_SWAP_DEVICE);
	vid_printf("Device: %s\n",dev ? dev : "-none-");
	vid_printf("Enabled: %d\n",enabled);
	vid_printf("KFrames: %zu\n",kframes);
	vid_printf("UFrames: %zu\n",uframes);
	vid_printf("Swapped out: %zu\n",swappedOut);
	vid_printf("Swapped in: %zu\n",swappedIn);
	vid_printf("\n");
	vid_printf("Jobs:\n");
	for(n = sll_begin(&swapInJobs); n != NULL; n = n->next) {
		sSwapInJob *job = (sSwapInJob*)n->data;
		vid_printf("\tThread %d:%d:%s @ %p\n",job->thread->tid,job->thread->proc->pid,
				job->thread->proc->command,job->addr);
	}
}
