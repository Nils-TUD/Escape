/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <esc/ipc/ipcbuf.h>
#include <esc/util.h>
#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <mem/physmemareas.h>
#include <mem/swapmap.h>
#include <mem/virtmem.h>
#include <sys/messages.h>
#include <task/proc.h>
#include <task/thread.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <boot.h>
#include <common.h>
#include <config.h>
#include <errno.h>
#include <log.h>
#include <spinlock.h>
#include <string.h>
#include <util.h>
#include <video.h>

//#define DEBUG_ALLOCS

#ifdef DEBUG_ALLOCS
#	define printAllocFree(fmt,...)	\
 	Util::printEventTrace(Log::get(),Util::getKernelStackTrace(),fmt,##__VA_ARGS__);
#else
#	define printAllocFree(...)
#endif

size_t PhysMem::totalMem = 0;

/* the bitmap for the frames of the lowest few MB; 0 = free, 1 = used */
tBitmap *PhysMem::bitmap;
uintptr_t PhysMem::bitmapStart;
size_t PhysMem::freeCont = 0;
SpinLock PhysMem::contLock;

/* We use a stack for the remaining memory
 * TODO Currently we don't free the frames for the stack */
PhysMem::StackFrames PhysMem::lower;
PhysMem::StackFrames PhysMem::upper;
SpinLock PhysMem::defLock;

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

extern void *_ebss;

uintptr_t PhysMem::bitmapStartFrame() {
	return PhysMem::bitmapStart / PAGE_SIZE;
}
uintptr_t PhysMem::lowerStart() {
	return (bitmapStartFrame() + BITMAP_PAGE_COUNT) * PAGE_SIZE;
}
uintptr_t PhysMem::lowerEnd() {
	return DIR_MAP_AREA_SIZE;
}

void PhysMem::init() {
	/* walk through the memory-map and mark all free areas as free */
	const Boot::Info *info = Boot::getInfo();
	for(size_t i = 0; i < info->mmapCount; ++i) {
		if(info->mmap[i].type == Boot::MemMap::MEM_AVAILABLE) {
			/* take care that we don't add memory that we can't access */
			if(info->mmap[i].baseAddr > (1ULL << PHYS_BITS) - 1) {
				Log::get().writef("Skipping unaddressable memory: %#Lx .. %#Lx\n",
					info->mmap[i].baseAddr,info->mmap[i].baseAddr + info->mmap[i].size);
				continue;
			}
			uint64_t end = info->mmap[i].baseAddr + info->mmap[i].size;
			if(end >= (1ULL << PHYS_BITS) - 1) {
				Log::get().writef("Skipping unaddressable memory: %#Lx .. %#Lx\n",
					(1ULL << PHYS_BITS) - 1,end);
				end = (1ULL << PHYS_BITS) - 1;
			}
			PhysMemAreas::add((uintptr_t)info->mmap[i].baseAddr,end);
		}
	}
	totalMem = PhysMemAreas::getAvailable();

	/* remove kernel and the first MB */
	PhysMemAreas::rem(0,(uintptr_t)&_ebss - KERNEL_BEGIN);

	/* remove modules */
	for(auto mod = Boot::modsBegin(); mod != Boot::modsEnd(); ++mod)
		PhysMemAreas::rem(mod->phys,mod->phys + mod->size);

	/* first, search memory that will be managed with the bitmap */
	const PhysMemAreas::MemArea *first = NULL;
	for(const PhysMemAreas::MemArea *area = PhysMemAreas::get(); area != NULL; area = area->next) {
		if(area->size >= BITMAP_PAGE_COUNT * PAGE_SIZE && (!first || area->addr < first->addr))
			first = area;
	}

	if(!first)
		Util::panic("Unable to find an area for the contiguous memory");

	/* map bitmap */
	size_t bmFrmCnt = BYTES_2_PAGES(BITMAP_PAGE_COUNT / 8);
	bitmap = (tBitmap*)PageDir::makeAccessible(0,bmFrmCnt);
	/* mark all free */
	memclear(bitmap,BITMAP_PAGE_COUNT / 8);
	bitmapStart = first->addr;
	freeCont = BITMAP_PAGE_COUNT;
	/* remove it from phys mem areas */
	PhysMemAreas::rem(first->addr,first->addr + BITMAP_PAGE_COUNT * PAGE_SIZE);

	/* determine which of the memory areas becomes lower and which upper memory */
	size_t lowerPages = 0,upperPages = 0;
	for(const PhysMemAreas::MemArea *area = PhysMemAreas::get(); area != NULL; area = area->next) {
		uintptr_t aend = area->addr + area->size;
		if(area->addr >= lowerStart() || aend < lowerStart())
			lowerPages += (esc::Util::min(lowerEnd(),aend) - area->addr) / PAGE_SIZE;
		if(aend > lowerEnd())
			upperPages += (aend - esc::Util::max(lowerEnd(),area->addr)) / PAGE_SIZE;
	}

	lower.pages = BYTES_2_PAGES(lowerPages * sizeof(frameno_t));
	upper.pages = BYTES_2_PAGES(upperPages * sizeof(frameno_t));

	/* map it so that we can access it; this will automatically remove some frames from the
	 * available memory. */
	lower.begin = (frameno_t*)PageDir::makeAccessible(0,lower.pages);
	lower.frames = lower.begin;
	if(upper.pages > 0) {
		upper.begin = (frameno_t*)PageDir::makeAccessible(0,upper.pages);
		upper.frames = upper.begin;
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
		swapEnabled = true;
	}

	/* determine kernel-memory-size */
	size_t free = getFreeDef();
	cframes = 0;
	kframes = free / (100 / KERNEL_MEM_PERCENT);
	if(kframes < KERNEL_MEM_MIN) {
		if(KERNEL_MEM_MIN > free) {
			kframes = free / 2;
			cframes = free - kframes;
		}
		else
			kframes = KERNEL_MEM_MIN;
	}
	if(cframes == 0) {
		cframes = (kframes * 2) / 3;
		kframes = kframes - cframes;
	}
	if(cframes + kframes > free / 2) {
		Log::get().writef("Warning: Detected VERY small number of free frames\n");
		Log::get().writef("         (%zu total, using %zu for kernel, %zu for critical)\n",
				free,kframes,cframes);
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
	LockGuard<SpinLock> g(&contLock);
	size_t c = 0;
	/* align in physical memory */
	size_t i = esc::Util::round_up(bitmapStartFrame(),align);
	i -= bitmapStartFrame();
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
		i = esc::Util::round_up(bitmapStartFrame() + j + 1,align);
		i -= bitmapStartFrame();
	}

	if(c != count)
		return -ENOMEM;

	/* the bitmap starts managing the memory at itself */
	i += bitmapStartFrame();
	doMarkRangeUsed(i * PAGE_SIZE,(i + count) * PAGE_SIZE,true);
	printAllocFree("[AC] %x:%zu ",i,count);
	return i;
}

void PhysMem::freeContiguous(frameno_t first,size_t count) {
	LockGuard<SpinLock> g(&contLock);
	printAllocFree("[FC] %x:%zu ",first,count);
	doMarkRangeUsed(first * PAGE_SIZE,(first + count) * PAGE_SIZE,false);
}

bool PhysMem::reserve(size_t frameCount,bool swap) {
	defLock.down();
	size_t free = getFreeDef();
	uframes += frameCount;
	/* enough user-memory available? */
	if(free >= frameCount && free - frameCount >= kframes + cframes) {
		defLock.up();
		return true;
	}

	/* swapping not possible? */
	Thread *t = Thread::getRunning();
	if(!swap || !swapEnabled || !swapperThread || t->getTid() == swapperThread->getTid()) {
		defLock.up();
		uframes -= frameCount;
		return false;
	}

	/* wait until we have <frameCount> frames free */
	do {
		/* notify swapper-thread */
		if(!swapping)
			Sched::wakeup(EV_SWAP_WORK,0);
		t->wait(EV_SWAP_FREE,0);
		defLock.up();
		Thread::switchNoSigs();
		defLock.down();
		free = getFreeDef();
	}
	while(free - (kframes + cframes) < frameCount);
	defLock.up();
	return true;
}

frameno_t PhysMem::allocFrame(bool forceLower) {
	/* prefer lower pages */
	if(!forceLower && (size_t)(lower.frames - lower.begin) <= kframes) {
		if(upper.frames == upper.begin)
			return PhysMem::INVALID_FRAME;
		return *(--upper.frames);
	}
	return *(--lower.frames);
}

void PhysMem::freeFrame(frameno_t frame) {
	if(frame * PAGE_SIZE < lowerEnd()) {
		if(lower.frames >= lower.begin + lower.pages * PAGE_SIZE)
			Util::panic("MM-Stack (lower) too small for physical memory!");
		*(lower.frames++) = frame;
	}
	else {
		if(upper.frames >= upper.begin + upper.pages * PAGE_SIZE)
			Util::panic("MM-Stack (upper) too small for physical memory!");
		*(upper.frames++) = frame;
	}
}

frameno_t PhysMem::allocate(FrameType type) {
	LockGuard<SpinLock> g(&defLock);
	/* remove the memory from the available one when we're not yet initialized */
	frameno_t frame = PhysMem::INVALID_FRAME;
	if(!initialized)
		frame = PhysMemAreas::alloc(1);
	else {
		switch(type) {
			case CRIT:
				/* if there are no cframes anymore, take away a few kframes */
				if(cframes == 0) {
					cframes += kframes / 2;
					kframes -= cframes;
				}
				if(cframes > 0) {
					cframes--;
					frame = allocFrame(false);
				}
				break;

			case KERN:
				/* if there are no kframes anymore, take away a few uframes */
				if(kframes == 0) {
					size_t free = lower.frames - lower.begin;
					kframes = (free - cframes) / (100 / KERNEL_MEM_PERCENT);
				}
				if(kframes > 0) {
					kframes--;
					frame = allocFrame(true);
				}
				break;

			default:
				if(getFreeDef() > (kframes + cframes)) {
					assert(uframes > 0);
					uframes--;
					frame = allocFrame(false);
				}
				break;
		}
	}
	printAllocFree("[A] %x 1 ",frame);
	return frame;
}

void PhysMem::free(frameno_t frame,FrameType type) {
	LockGuard<SpinLock> g(&defLock);
	printAllocFree("[F] %x 1 ",frame);
	if(type == CRIT)
		cframes++;
	else if(type == KERN)
		kframes++;
	markUsed(frame,false);
}

int PhysMem::swapIn(uintptr_t addr) {
	if(!swapEnabled)
		return -EFAULT;

	/* get a free job */
	Thread *t = Thread::getRunning();
	defLock.down();
	SwapInJob *job;
	do {
		job = siFreelist;
		if(job == NULL) {
			t->wait(EV_SWAP_JOB,0);
			jobWaiters++;
			defLock.up();
			Thread::switchNoSigs();
			defLock.down();
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
		Sched::wakeup(EV_SWAP_WORK,0);
	/* wait until its done */
	t->block();
	defLock.up();
	Thread::switchNoSigs();
	return 0;
}

void PhysMem::swapper() {
	const char *dev = Config::getStr(Config::SWAP_DEVICE);
	swapperThread = Thread::getRunning();
	pid_t pid = swapperThread->getProc()->getPid();
	assert(swapEnabled);

	/* open device */
	OpenFile *swapFile;
	if(VFS::openPath(pid,VFS_READ | VFS_WRITE,0,dev,NULL,&swapFile) < 0) {
		Log::get().writef("Unable to open swap-device '%s'\n",dev);
		swapEnabled = false;
	}
	else {
		/* get device-size and init swap-map */
		struct stat info;
		sassert(swapFile->fstat(pid,&info) == 0);

		if(!SwapMap::init(info.st_size)) {
			swapEnabled = false;
			swapFile->close(pid);
		}
	}

	/* start main-loop; wait for work */
	defLock.down();
	while(1) {
		SwapInJob *job;
		size_t free = getFreeDef();
		/* swapping out is more important than swapping in */
		if((free - (kframes + cframes)) < uframes) {
			size_t amount = esc::Util::min(MAX_SWAP_AT_ONCE,uframes - (free - (kframes + cframes)));
			swapping = true;
			defLock.up();

			VirtMem::swapOut(pid,swapFile,amount);
			swappedOut += amount;

			defLock.down();
			swapping = false;
		}
		/* wakeup in every case because its possible that the frames are available now but weren't
		 * previously */
		Sched::wakeup(EV_SWAP_FREE,0);

		/* handle swap-in-jobs */
		while((job = getJob()) != NULL) {
			swapping = true;
			defLock.up();

			if(VirtMem::swapIn(pid,swapFile,job->thread,job->addr))
				swappedIn++;

			defLock.down();
			job->thread->unblock();
			freeJob(job);
			swapping = false;
		}

		if(getFreeDef() - (kframes + cframes) >= uframes) {
			/* we may receive new work now */
			swapperThread->wait(EV_SWAP_WORK,0);
			defLock.up();
			Thread::switchAway();
			defLock.down();
		}
	}
	defLock.up();
}

void PhysMem::print(OStream &os) {
	const char *dev = Config::getStr(Config::SWAP_DEVICE);
	os.writef("Default: %zu\n",getFreeDef());
	os.writef("Contiguous: %zu\n",freeCont);
	os.writef("Swap-Device: %s\n",dev ? dev : "-none-");
	os.writef("Swap enabled: %d\n",swapEnabled);
	os.writef("CFrames: %zu\n",cframes);
	os.writef("KFrames: %zu\n",kframes);
	os.writef("UFrames: %zu\n",uframes);
	os.writef("Swapped out: %zu\n",swappedOut);
	os.writef("Swapped in: %zu\n",swappedIn);
	os.writef("\n");
	os.writef("Swap-in-jobs:\n");
	for(SwapInJob *job = siJobList; job != NULL; job = job->next) {
		os.writef("\tThread %d:%d:%s @ %p\n",job->thread->getTid(),job->thread->getProc()->getPid(),
				job->thread->getProc()->getProgram(),job->addr);
	}
}

void PhysMem::printStack(OStream &os) {
	struct {
		const char *name;
		StackFrames *obj;
	} stacks[] = {
		{"Lower",&lower},
		{"Upper",&upper}
	};

	for(size_t i = 0; i < ARRAY_SIZE(stacks); ++i) {
		os.writef("%s stack: (frame numbers)\n",stacks[i].name);
		if(stacks[i].obj->frames != stacks[i].obj->begin) {
			frameno_t *ptr = stacks[i].obj->frames - 1;
			for(size_t j = 0; ptr >= stacks[i].obj->begin; j++, ptr--) {
				os.writef("0x%08Px, ",*ptr);
				if(j % 6 == 5)
					os.writef("\n");
			}
		}
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
	return (lower.frames - lower.begin) + (upper.frames - upper.begin);
}

void PhysMem::markRangeUsed(uintptr_t from,uintptr_t to,bool used) {
	doMarkRangeUsed(from,to,used);
}

void PhysMem::doMarkRangeUsed(uintptr_t from,uintptr_t to,bool used) {
	/* ensure that we start at a page-start */
	from &= ~(PAGE_SIZE - 1);
	for(; from < to; from += PAGE_SIZE)
		markUsed(from >> PAGE_BITS,used);
}

void PhysMem::markUsed(frameno_t frame,bool used) {
	/* ignore the stuff before; we don't manage it */
	if(frame < bitmapStartFrame())
		return;
	/* we use a bitmap for the lowest few MB */
	if(frame < bitmapStartFrame() + BITMAP_PAGE_COUNT) {
		tBitmap bit,*bitmapEntry;
		/* the bitmap starts managing the memory at itself */
		frame -= bitmapStartFrame();
		bitmapEntry = (tBitmap*)(bitmap + (frame / BITS_PER_BMWORD));
		bit = (BITS_PER_BMWORD - 1) - (frame % BITS_PER_BMWORD);
		if(used) {
			*bitmapEntry = *bitmapEntry | (1UL << bit);
			assert(freeCont > 0);
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
		if(!used)
			freeFrame(frame);
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
		Sched::wakeup(EV_SWAP_JOB,0);
}
