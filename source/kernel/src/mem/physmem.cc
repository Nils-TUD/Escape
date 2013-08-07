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
#include <sys/mem/physmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/physmemareas.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/event.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/config.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <sys/log.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <esc/messages.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#define printEventTrace(...)

#define BITMAP_START_FRAME			(PhysMem::bitmapStart / PAGE_SIZE)

/* the bitmap for the frames of the lowest few MB; 0 = free, 1 = used */
tBitmap *PhysMem::bitmap;
uintptr_t PhysMem::bitmapStart;
size_t PhysMem::freeCont = 0;

/* We use a stack for the remaining memory
 * TODO Currently we don't free the frames for the stack */
size_t PhysMem::stackPages = 0;
uintptr_t PhysMem::stackBegin = 0;
frameno_t *PhysMem::stack = NULL;
klock_t PhysMem::contLock;
klock_t PhysMem::defLock;
bool PhysMem::initialized = false;

/* for swapping */
size_t PhysMem::swappedOut = 0;
size_t PhysMem::swappedIn = 0;
bool PhysMem::swapEnabled = false;
bool PhysMem::swapping = false;
Thread *PhysMem::swapperThread = NULL;
size_t PhysMem::cframes = 0;	/* critical frames; for dynarea, cache and heap */
size_t PhysMem::kframes = 0;	/* kernel frames: for pagedirs, page-tables, kstacks, ... */
size_t PhysMem::uframes = 0;	/* user frames */
/* swap-in jobs */
PhysMem::SwapInJob PhysMem::siJobs[SWAPIN_JOB_COUNT];
PhysMem::SwapInJob *PhysMem::siFreelist;
PhysMem::SwapInJob *PhysMem::siJobList = NULL;
PhysMem::SwapInJob *PhysMem::siJobEnd = NULL;
size_t PhysMem::jobWaiters = 0;

void PhysMem::init() {
	/* determine the available memory */
	PhysMemAreas::initArch();

	/* calculate mm-stack-size */
	size_t memSize = PhysMemAreas::getAvailable();
	size_t defPageCount = (memSize / PAGE_SIZE) - BITMAP_PAGE_COUNT;
	stackPages = (defPageCount + (PAGE_SIZE - 1) / sizeof(frameno_t)) / (PAGE_SIZE / sizeof(frameno_t));

	/* map it so that we can access it; this will automatically remove some frames from the
	 * available memory. */
	stackBegin = PageDir::makeAccessible(0,stackPages);
	stack = (frameno_t*)stackBegin;

	/* map bitmap behind it */
	size_t bmFrmCnt = BYTES_2_PAGES(BITMAP_PAGE_COUNT / 8);
	bitmap = (tBitmap*)PageDir::makeAccessible(0,bmFrmCnt);
	/* mark all free */
	memclear(bitmap,BITMAP_PAGE_COUNT / 8);

	/* first, search memory that will be managed with the bitmap */
	for(const PhysMemAreas::MemArea *area = PhysMemAreas::get(); area != NULL; area = area->next) {
		if(area->size >= BITMAP_PAGE_COUNT * PAGE_SIZE) {
			bitmapStart = area->addr;
			PhysMemAreas::rem(area->addr,area->addr + BITMAP_PAGE_COUNT * PAGE_SIZE);
			break;
		}
	}

	/* now mark the remaining memory as free on stack */
	for(const PhysMemAreas::MemArea *area = PhysMemAreas::get(); area != NULL; area = area->next)
		markRangeUsed(area->addr,area->addr + area->size,false);

	/* stack and bitmap is ready */
	initialized = true;

	/* test whether a swap-device is present */
	if(Config::getStr(Config::SWAP_DEVICE) != NULL) {
		/* build freelist */
		siFreelist = siJobs + 0;
		siJobs[0].next = NULL;
		for(size_t i = 1; i < SWAPIN_JOB_COUNT; i++) {
			siJobs[i].next = siFreelist;
			siFreelist = siJobs + i;
		}

		/* determine kernel-memory-size */
		size_t free = getFreeDef();
		kframes = free / (100 / KERNEL_MEM_PERCENT);
		if(kframes < KERNEL_MEM_MIN)
			kframes = KERNEL_MEM_MIN;
		cframes = (kframes * 2) / 3;
		kframes = kframes - cframes;
		if(cframes + kframes > free / 2) {
			Log::get().writef("Warning: Detected VERY small number of free frames\n");
			Log::get().writef("         (%zu total, using %zu for kernel, %zu for critical)\n",
					free,kframes,cframes);
		}
		swapEnabled = true;
	}
}

size_t PhysMem::getFreeFrames(uint types) {
	/* no lock; just intended for debugging and information */
	size_t count = 0;
	if(types & CONT)
		count += freeCont;
	if(types & DEF)
		count += getFreeDef();
	return count;
}

ssize_t PhysMem::allocateContiguous(size_t count,size_t align) {
	size_t c = 0;
	SpinLock::acquire(&contLock);
	/* align in physical memory */
	size_t i = ROUND_UP(BITMAP_START_FRAME,align);
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
		i = ROUND_UP(BITMAP_START_FRAME + j + 1,align);
		i -= BITMAP_START_FRAME;
	}

	if(c != count) {
		SpinLock::release(&contLock);
		return -ENOMEM;
	}

	/* the bitmap starts managing the memory at itself */
	i += BITMAP_START_FRAME;
	doMarkRangeUsed(i * PAGE_SIZE,(i + count) * PAGE_SIZE,true);
	printEventTrace(Util::getKernelStackTrace(),"[AC] %x:%zu ",i,count);
	SpinLock::release(&contLock);
	return i;
}

void PhysMem::freeContiguous(frameno_t first,size_t count) {
	SpinLock::acquire(&contLock);
	printEventTrace(Util::getKernelStackTrace(),"[FC] %x:%zu ",first,count);
	doMarkRangeUsed(first * PAGE_SIZE,(first + count) * PAGE_SIZE,false);
	SpinLock::release(&contLock);
}

bool PhysMem::reserve(size_t frameCount) {
	SpinLock::acquire(&defLock);
	size_t free = getFreeDef();
	uframes += frameCount;
	/* enough user-memory available? */
	if(free >= frameCount && free - frameCount >= kframes + cframes) {
		SpinLock::release(&defLock);
		return true;
	}

	/* swapping not possible? */
	Thread *t = Thread::getRunning();
	if(!swapEnabled || t->getTid() == ATA_TID || t->getTid() == swapperThread->getTid()) {
		SpinLock::release(&defLock);
		return false;
	}

	/* wait until we have <frameCount> frames free */
	do {
		/* notify swapper-thread */
		if(!swapping)
			Event::wakeupThread(swapperThread,EV_SWAP_WORK);
		Event::wait(t,EVI_SWAP_FREE,0);
		SpinLock::release(&defLock);
		Thread::switchNoSigs();
		SpinLock::acquire(&defLock);
		free = getFreeDef();
	}
	while(free - (kframes + cframes) < frameCount);
	SpinLock::release(&defLock);
	return true;
}

frameno_t PhysMem::allocate(FrameType type) {
	frameno_t frm = 0;
	SpinLock::acquire(&defLock);
	printEventTrace(Util::getKernelStackTrace(),"[A] %x ",*(stack - 1));
	/* remove the memory from the available one when we're not yet initialized */
	if(!initialized)
		frm = PhysMemAreas::alloc(1);
	else if(swapEnabled && type == CRIT) {
		if(cframes > 0) {
			frm = *(--stack);
			cframes--;
		}
	}
	else if(swapEnabled && type == KERN) {
		/* if there are no kframes anymore, take away a few uframes */
		if(kframes == 0) {
			size_t free = getFreeDef();
			kframes = (free - cframes) / (100 / KERNEL_MEM_PERCENT);
		}
		/* return 0 otherwise */
		if(kframes > 0) {
			frm = *(--stack);
			kframes--;
		}
	}
	else {
		size_t free = getFreeDef();
		if(free > (kframes + cframes)) {
			assert(type != USR || uframes > 0);
			frm = *(--stack);
			if(type == USR)
				uframes--;
		}
	}
	SpinLock::release(&defLock);
	return frm;
}

void PhysMem::free(frameno_t frame,FrameType type) {
	SpinLock::acquire(&defLock);
	printEventTrace(Util::getKernelStackTrace(),"[F] %x ",frame);
	if(swapEnabled) {
		if(type == CRIT)
			cframes++;
		else if(type == KERN)
			kframes++;
	}
	markUsed(frame,false);
	SpinLock::release(&defLock);
}

bool PhysMem::swapIn(uintptr_t addr) {
	if(!swapEnabled)
		return false;

	/* get a free job */
	Thread *t = Thread::getRunning();
	SpinLock::acquire(&defLock);
	SwapInJob *job;
	do {
		job = siFreelist;
		if(job == NULL) {
			Event::wait(t,EVI_SWAP_JOB,0);
			jobWaiters++;
			SpinLock::release(&defLock);
			Thread::switchNoSigs();
			SpinLock::acquire(&defLock);
			jobWaiters--;
		}
	}
	while(job == NULL);
	siFreelist = siFreelist->next;

	/* append the job */
	job->addr = addr;
	job->thread = t;
	appendJob(job);

	/* notify the swapper, if necessary */
	if(!swapping)
		Event::wakeupThread(swapperThread,EV_SWAP_WORK);
	/* wait until its done */
	Event::block(t);
	SpinLock::release(&defLock);
	Thread::switchNoSigs();
	return true;
}

void PhysMem::swapper() {
	const char *dev = Config::getStr(Config::SWAP_DEVICE);
	swapperThread = Thread::getRunning();
	pid_t pid = swapperThread->getProc()->getPid();
	assert(swapEnabled);

	/* open device */
	OpenFile *swapFile;
	if(VFS::openPath(pid,VFS_READ | VFS_WRITE | VFS_MSGS,dev,&swapFile) < 0) {
		Log::get().writef("Unable to open swap-device '%s'\n",dev);
		swapEnabled = false;
	}
	else {
		/* get device-size and init swap-map */
		sArgsMsg msg;
		assert(swapFile->sendMsg(pid,MSG_DISK_GETSIZE,NULL,0,NULL,0) >= 0);
		assert(swapFile->receiveMsg(pid,NULL,&msg,sizeof(msg),false) >= 0);
		if(!SwapMap::init(msg.arg1)) {
			swapEnabled = false;
			swapFile->closeFile(pid);
		}
	}

	/* start main-loop; wait for work */
	SpinLock::acquire(&defLock);
	while(1) {
		SwapInJob *job;
		size_t free = getFreeDef();
		/* swapping out is more important than swapping in */
		if((free - (kframes + cframes)) < uframes) {
			size_t amount = MIN(MAX_SWAP_AT_ONCE,uframes - (free - (kframes + cframes)));
			swapping = true;
			SpinLock::release(&defLock);

			VirtMem::swapOut(pid,swapFile,amount);
			swappedOut += amount;

			SpinLock::acquire(&defLock);
			swapping = false;
		}
		/* wakeup in every case because its possible that the frames are available now but weren't
		 * previously */
		Event::wakeup(EVI_SWAP_FREE,0);

		/* handle swap-in-jobs */
		while((job = getJob()) != NULL) {
			swapping = true;
			SpinLock::release(&defLock);

			if(VirtMem::swapIn(pid,swapFile,job->thread,job->addr))
				swappedIn++;

			SpinLock::acquire(&defLock);
			Event::unblock(job->thread);
			freeJob(job);
			swapping = false;
		}

		if(getFreeDef() - (kframes + cframes) >= uframes) {
			/* we may receive new work now */
			Event::wait(swapperThread,EVI_SWAP_WORK,0);
			SpinLock::release(&defLock);
			Thread::switchAway();
			SpinLock::acquire(&defLock);
		}
	}
	SpinLock::release(&defLock);
}

void PhysMem::print(OStream &os) {
	const char *dev = Config::getStr(Config::SWAP_DEVICE);
	os.writef("Default: %zu\n",getFreeDef());
	os.writef("Contiguous: %zu\n",freeCont);
	os.writef("Swap-Device: %s\n",dev ? dev : "-none-");
	os.writef("Swap enabled: %d\n",swapEnabled);
	os.writef("EFrames: %zu\n",cframes);
	os.writef("KFrames: %zu\n",kframes);
	os.writef("UFrames: %zu\n",uframes);
	os.writef("Swapped out: %zu\n",swappedOut);
	os.writef("Swapped in: %zu\n",swappedIn);
	os.writef("\n");
	os.writef("Swap-in-jobs:\n");
	for(SwapInJob *job = siJobList; job != NULL; job = job->next) {
		os.writef("\tThread %d:%d:%s @ %p\n",job->thread->getTid(),job->thread->getProc()->getPid(),
				job->thread->getProc()->getCommand(),job->addr);
	}
}

void PhysMem::printStack(OStream &os) {
	os.writef("Stack: (frame numbers)\n");
	frameno_t *ptr;
	size_t i;
	for(i = 0, ptr = stack - 1; (uintptr_t)ptr >= stackBegin; i++, ptr--) {
		os.writef("0x%08Px, ",*ptr);
		if(i % 6 == 5)
			os.writef("\n");
	}
}

void PhysMem::printCont(OStream &os) {
	os.writef("Bitmap: (frame numbers)\n");
	size_t pos = bitmapStart;
	for(size_t i = 0; i < BITMAP_PAGE_COUNT / BITS_PER_BMWORD; i++) {
		os.writef("0x%08Px..: %0*lb\n",pos / PAGE_SIZE,sizeof(tBitmap) * 8,bitmap[i]);
		pos += PAGE_SIZE * BITS_PER_BMWORD;
	}
}

size_t PhysMem::getFreeDef() {
	return ((uintptr_t)stack - stackBegin) / sizeof(frameno_t);
}

void PhysMem::markRangeUsed(uintptr_t from,uintptr_t to,bool used) {
	doMarkRangeUsed(from,to,used);
}

void PhysMem::doMarkRangeUsed(uintptr_t from,uintptr_t to,bool used) {
	/* ensure that we start at a page-start */
	from &= ~(PAGE_SIZE - 1);
	for(; from < to; from += PAGE_SIZE)
		markUsed(from >> PAGE_SIZE_SHIFT,used);
}

void PhysMem::markUsed(frameno_t frame,bool used) {
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
			if((uintptr_t)stack >= stackBegin + stackPages * PAGE_SIZE)
				Util::panic("MM-Stack too small for physical memory!");
			*stack = frame;
			stack++;
		}
	}
}

void PhysMem::appendJob(SwapInJob *job) {
	job->next = NULL;
	if(siJobEnd)
		siJobEnd->next = job;
	else
		siJobList = job;
	siJobEnd = job;
}

PhysMem::SwapInJob *PhysMem::getJob() {
	SwapInJob *res;
	if(!siJobList)
		return NULL;
	res = siJobList;
	siJobList = siJobList->next;
	if(!siJobList)
		siJobEnd = NULL;
	return res;
}

void PhysMem::freeJob(SwapInJob *job) {
	job->next = siFreelist;
	siFreelist = job;
	if(jobWaiters > 0)
		Event::wakeup(EVI_SWAP_JOB,0);
}
