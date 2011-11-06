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
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/sllnodes.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/vmm.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/vfs/vfs.h>
#include <sys/config.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <esc/messages.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#define util_printEventTrace(...)

#define BITS_PER_BMWORD				(sizeof(tBitmap) * 8)
#define BITMAP_START_FRAME			(BITMAP_START / PAGE_SIZE)

#define KERNEL_MEM_PERCENT			20
#define KERNEL_MEM_MIN				750
#define MAX_SWAP_AT_ONCE			10
#define SWAPIN_JOB_COUNT			64

typedef struct sSwapInJob {
	uintptr_t addr;
	sThread *thread;
	struct sSwapInJob *next;
} sSwapInJob;

static size_t pmem_getFreeDef(void);
static void pmem_doMarkRangeUsed(uintptr_t from,uintptr_t to,bool used);
static void pmem_markUsed(frameno_t frame,bool used);
static void pmem_appendJob(sSwapInJob *job);
static sSwapInJob *pmem_getJob(void);
static void pmem_freeJob(sSwapInJob *job);

/* the bitmap for the frames of the lowest few MB; 0 = free, 1 = used */
static tBitmap *bitmap;
static size_t freeCont = 0;

/* We use a stack for the remaining memory
 * TODO Currently we don't free the frames for the stack */
static size_t stackSize = 0;
static uintptr_t stackBegin = 0;
static frameno_t *stack = NULL;
static klock_t contLock;
static klock_t defLock;
static bool initializing;

/* for swapping */
static size_t swappedOut = 0;
static size_t swappedIn = 0;
static bool swapEnabled = false;
static bool swapping = false;
static sThread *swapper = NULL;
static size_t cframes = 0;	/* critical frames; for dynarea, cache and heap */
static size_t kframes = 0;	/* kernel frames: for pagedirs, page-tables, kstacks, ... */
static size_t uframes = 0;	/* user frames */
/* swap-in jobs */
static sSwapInJob siJobs[SWAPIN_JOB_COUNT];
static sSwapInJob *siFreelist;
static sSwapInJob *siJobList = NULL;
static sSwapInJob *siJobEnd = NULL;
static size_t jobWaiters = 0;

void pmem_init(void) {
	size_t i,free;
	initializing = true;
	pmem_initArch(&stackBegin,&stackSize,&bitmap);
	stack = (frameno_t*)stackBegin;
	pmem_markAvailable();
	initializing = false;

	/* test whether a swap-device is present */
	if(conf_getStr(CONF_SWAP_DEVICE) != NULL) {
		/* build freelist */
		siFreelist = siJobs + 0;
		siJobs[0].next = NULL;
		for(i = 1; i < SWAPIN_JOB_COUNT; i++) {
			siJobs[i].next = siFreelist;
			siFreelist = siJobs + i;
		}

		/* determine kernel-memory-size */
		free = pmem_getFreeDef();
		kframes = free / (100 / KERNEL_MEM_PERCENT);
		if(kframes < KERNEL_MEM_MIN)
			kframes = KERNEL_MEM_MIN;
		cframes = (kframes * 2) / 3;
		kframes = kframes - cframes;
		if(cframes + kframes > free / 2) {
			log_printf("Warning: Detected VERY small number of free frames\n");
			log_printf("         (%zu total, using %zu for kernel, %zu for critical)\n",
					free,kframes,cframes);
		}
		swapEnabled = true;
	}
}

bool pmem_canSwap(void) {
	return swapEnabled;
}

size_t pmem_getStackSize(void) {
	return stackSize * PAGE_SIZE;
}

size_t pmem_getFreeFrames(uint types) {
	/* no lock; just intended for debugging and information */
	size_t count = 0;
	if(types & MM_CONT)
		count += freeCont;
	if(types & MM_DEF)
		count += pmem_getFreeDef();
	return count;
}

ssize_t pmem_allocateContiguous(size_t count,size_t align) {
	size_t i,c = 0;
	spinlock_aquire(&contLock);
	/* align in physical memory */
	i = (BITMAP_START_FRAME + align - 1) & ~(align - 1);
	i -= BITMAP_START_FRAME;
	for(; i < BITMAP_PAGE_COUNT; ) {
		/* walk forward until we find an occupied frame */
		size_t j = i;
		for(c = 0; c < count; j++,c++) {
			tBitmap dword = bitmap[j / BITS_PER_BMWORD];
			tBitmap bit = (BITS_PER_BMWORD - 1) - (j % BITS_PER_BMWORD);
			if(dword & (1UL << bit))
				break;
		}
		/* found enough? */
		if(c == count)
			break;
		/* ok, to next aligned frame */
		i = (BITMAP_START_FRAME + j + 1 + align - 1) & ~(align - 1);
		i -= BITMAP_START_FRAME;
	}

	if(c != count) {
		spinlock_release(&contLock);
		return -ENOMEM;
	}

	/* the bitmap starts managing the memory at itself */
	i += BITMAP_START_FRAME;
	pmem_doMarkRangeUsed(i * PAGE_SIZE,(i + count) * PAGE_SIZE,true);
	util_printEventTrace(util_getKernelStackTrace(),"[AC] %x:%zu ",i,count);
	spinlock_release(&contLock);
	return i;
}

void pmem_freeContiguous(frameno_t first,size_t count) {
	spinlock_aquire(&contLock);
	util_printEventTrace(util_getKernelStackTrace(),"[FC] %x:%zu ",first,count);
	pmem_doMarkRangeUsed(first * PAGE_SIZE,(first + count) * PAGE_SIZE,false);
	spinlock_release(&contLock);
}

bool pmem_reserve(size_t frameCount) {
	size_t free;
	sThread *t;
	spinlock_aquire(&defLock);
	free = pmem_getFreeDef();
	uframes += frameCount;
	/* enough user-memory available? */
	if(free >= frameCount && free - frameCount >= kframes + cframes) {
		spinlock_release(&defLock);
		return true;
	}

	/* swapping not possible? */
	t = thread_getRunning();
	if(!swapEnabled || t->tid == ATA_TID || t->tid == swapper->tid) {
		spinlock_release(&defLock);
		return false;
	}

	/* wait until we have <frameCount> frames free */
	do {
		/* notify swapper-thread */
		if(!swapping)
			ev_wakeupThread(swapper,EV_SWAP_WORK);
		ev_wait(t,EVI_SWAP_FREE,0);
		spinlock_release(&defLock);
		thread_switchNoSigs();
		spinlock_aquire(&defLock);
		free = pmem_getFreeDef();
	}
	while(free - (kframes + cframes) < frameCount);
	spinlock_release(&defLock);
	return true;
}

frameno_t pmem_allocate(eFrmType type) {
	frameno_t frm = 0;
	spinlock_aquire(&defLock);
	util_printEventTrace(util_getKernelStackTrace(),"[A] %x ",*(stack - 1));
	if(swapEnabled && type == FRM_CRIT) {
		if(cframes > 0) {
			frm = *(--stack);
			cframes--;
		}
	}
	else if(swapEnabled && type == FRM_KERNEL) {
		/* if there are no kframes anymore, take away a few uframes */
		if(kframes == 0) {
			size_t free = pmem_getFreeDef();
			kframes = (free - cframes) / (100 / KERNEL_MEM_PERCENT);
		}
		/* return 0 otherwise */
		if(kframes > 0) {
			frm = *(--stack);
			kframes--;
		}
	}
	else {
		size_t free = pmem_getFreeDef();
		if(free > (kframes + cframes)) {
			assert(type != FRM_USER || uframes > 0);
			frm = *(--stack);
			if(type == FRM_USER)
				uframes--;
		}
	}
	spinlock_release(&defLock);
	return frm;
}

void pmem_free(frameno_t frame,eFrmType type) {
	spinlock_aquire(&defLock);
	util_printEventTrace(util_getKernelStackTrace(),"[F] %x ",frame);
	if(swapEnabled) {
		if(type == FRM_CRIT)
			cframes++;
		else if(type == FRM_KERNEL)
			kframes++;
	}
	pmem_markUsed(frame,false);
	spinlock_release(&defLock);
}

bool pmem_swapIn(uintptr_t addr) {
	sThread *t;
	sSwapInJob *job;
	if(!swapEnabled)
		return false;

	/* get a free job */
	t = thread_getRunning();
	spinlock_aquire(&defLock);
	do {
		job = siFreelist;
		if(job == NULL) {
			ev_wait(t,EVI_SWAP_JOB,0);
			jobWaiters++;
			spinlock_release(&defLock);
			thread_switchNoSigs();
			spinlock_aquire(&defLock);
			jobWaiters--;
		}
	}
	while(job == NULL);
	siFreelist = siFreelist->next;

	/* append the job */
	job->addr = addr;
	job->thread = t;
	pmem_appendJob(job);

	/* notify the swapper, if necessary */
	if(!swapping)
		ev_wakeupThread(swapper,EV_SWAP_WORK);
	/* wait until its done */
	ev_block(t);
	spinlock_release(&defLock);
	thread_switchNoSigs();
	return true;
}

void pmem_swapper(void) {
	size_t free;
	file_t swapFile;
	pid_t pid;
	const char *dev = conf_getStr(CONF_SWAP_DEVICE);
	swapper = thread_getRunning();
	pid = swapper->proc->pid;
	assert(swapEnabled);

	/* open device */
	swapFile = vfs_openPath(pid,VFS_READ | VFS_WRITE | VFS_MSGS,dev);
	if(swapFile < 0) {
		log_printf("Unable to open swap-device '%s'\n",dev);
		swapEnabled = false;
	}
	else {
		/* get device-size and init swap-map */
		sArgsMsg msg;
		assert(vfs_sendMsg(pid,swapFile,MSG_DISK_GETSIZE,NULL,0,NULL,0) >= 0);
		assert(vfs_receiveMsg(pid,swapFile,NULL,&msg,sizeof(msg),false) >= 0);
		if(!swmap_init(msg.arg1)) {
			swapEnabled = false;
			vfs_closeFile(pid,swapFile);
		}
	}

	/* start main-loop; wait for work */
	spinlock_aquire(&defLock);
	while(1) {
		sSwapInJob *job;
		free = pmem_getFreeDef();
		/* swapping out is more important than swapping in */
		if((free - (kframes + cframes)) < uframes) {
			size_t amount = MIN(MAX_SWAP_AT_ONCE,uframes - (free - (kframes + cframes)));
			swapping = true;
			spinlock_release(&defLock);

			vmm_swapOut(pid,swapFile,amount);
			swappedOut += amount;

			spinlock_aquire(&defLock);
			swapping = false;
		}
		/* wakeup in every case because its possible that the frames are available now but weren't
		 * previously */
		ev_wakeup(EVI_SWAP_FREE,0);

		/* handle swap-in-jobs */
		while((job = pmem_getJob()) != NULL) {
			swapping = true;
			spinlock_release(&defLock);

			if(vmm_swapIn(pid,swapFile,job->thread,job->addr))
				swappedIn++;

			spinlock_aquire(&defLock);
			ev_unblock(job->thread);
			pmem_freeJob(job);
			swapping = false;
		}

		if(pmem_getFreeDef() - (kframes + cframes) >= uframes) {
			/* we may receive new work now */
			ev_wait(swapper,EVI_SWAP_WORK,0);
			spinlock_release(&defLock);
			thread_switch();
			spinlock_aquire(&defLock);
		}
	}
	spinlock_release(&defLock);
}

void pmem_markRangeUsed(uintptr_t from,uintptr_t to,bool used) {
	assert(initializing);
	pmem_doMarkRangeUsed(from,to,used);
}

void pmem_print(void) {
	sSwapInJob *job;
	const char *dev = conf_getStr(CONF_SWAP_DEVICE);
	vid_printf("Default: %zu\n",pmem_getFreeDef());
	vid_printf("Contiguous: %zu\n",freeCont);
	vid_printf("Swap-Device: %s\n",dev ? dev : "-none-");
	vid_printf("Swap enabled: %d\n",swapEnabled);
	vid_printf("EFrames: %zu\n",cframes);
	vid_printf("KFrames: %zu\n",kframes);
	vid_printf("UFrames: %zu\n",uframes);
	vid_printf("Swapped out: %zu\n",swappedOut);
	vid_printf("Swapped in: %zu\n",swappedIn);
	vid_printf("\n");
	vid_printf("Swap-in-jobs:\n");
	for(job = siJobList; job != NULL; job = job->next) {
		vid_printf("\tThread %d:%d:%s @ %p\n",job->thread->tid,job->thread->proc->pid,
				job->thread->proc->command,job->addr);
	}
}

void pmem_printStack(void) {
	size_t i = BITMAP_START;
	frameno_t *ptr;
	vid_printf("Stack: (frame numbers)\n");
	for(i = 0,ptr = stack - 1;(uintptr_t)ptr >= stackBegin; i++,ptr--) {
		vid_printf("0x%08Px, ",*ptr);
		if(i % 6 == 5)
			vid_printf("\n");
	}
}

void pmem_printCont(void) {
	size_t i,pos = BITMAP_START;
	vid_printf("Bitmap: (frame numbers)\n");
	for(i = 0; i < BITMAP_PAGE_COUNT / BITS_PER_BMWORD; i++) {
		vid_printf("0x%08Px..: %0*lb\n",pos / PAGE_SIZE,sizeof(tBitmap) * 8,bitmap[i]);
		pos += PAGE_SIZE * BITS_PER_BMWORD;
	}
}

static size_t pmem_getFreeDef(void) {
	return ((uintptr_t)stack - stackBegin) / sizeof(frameno_t);
}

static void pmem_doMarkRangeUsed(uintptr_t from,uintptr_t to,bool used) {
	/* ensure that we start at a page-start */
	from &= ~(PAGE_SIZE - 1);
	for(; from < to; from += PAGE_SIZE)
		pmem_markUsed(from >> PAGE_SIZE_SHIFT,used);
}

static void pmem_markUsed(frameno_t frame,bool used) {
	/* ignore the stuff before; we don't manage it */
	if(frame < BITMAP_START_FRAME)
		return;
	/* we use a bitmap for the lowest few MB */
	if(frame < BITMAP_START_FRAME + BITMAP_PAGE_COUNT) {
		tBitmap bit,*bitmapEntry;
		/* the bitmap starts managing the memory at itself */
		frame -= BITMAP_START_FRAME;
		bitmapEntry = (tBitmap*)(bitmap + (frame / BITS_PER_BMWORD));
		bit = (BITS_PER_BMWORD - 1) - (frame % BITS_PER_BMWORD);
		if(used) {
			*bitmapEntry = *bitmapEntry | (1UL << bit);
			freeCont--;
		}
		else {
			*bitmapEntry = *bitmapEntry & ~(1UL << bit);
			freeCont++;
		}
	}
	/* use a stack for the remaining memory */
	else {
		/* we don't mark frames as used since this function is just used for initializing the
		 * memory-management */
		if(!used) {
			if((uintptr_t)stack >= PMEM_END)
				util_panic("MM-Stack too small for physical memory!");
			*stack = frame;
			stack++;
		}
	}
}

static void pmem_appendJob(sSwapInJob *job) {
	job->next = NULL;
	if(siJobEnd)
		siJobEnd->next = job;
	else
		siJobList = job;
	siJobEnd = job;
}

static sSwapInJob *pmem_getJob(void) {
	sSwapInJob *res;
	if(!siJobList)
		return NULL;
	res = siJobList;
	siJobList = siJobList->next;
	if(!siJobList)
		siJobEnd = NULL;
	return res;
}

static void pmem_freeJob(sSwapInJob *job) {
	job->next = siFreelist;
	siFreelist = job;
	if(jobWaiters > 0)
		ev_wakeup(EVI_SWAP_JOB,0);
}
