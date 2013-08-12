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
#include <sys/mem/region.h>
#include <sys/mem/paging.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/copyonwrite.h>
#include <sys/mem/cache.h>
#include <sys/mem/swapmap.h>
#include <sys/task/event.h>
#include <sys/task/smp.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/cppsupport.h>
#include <sys/spinlock.h>
#include <sys/ostream.h>
#include <sys/mutex.h>
#include <sys/util.h>
#include <sys/log.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

/**
 * The vmm-module manages the user-part of a process's virtual addressspace. That means it
 * manages the regions that are used by the process (as an instance of VMRegion) and decides
 * where the regions are placed. So it binds a region to a virtual address via VMRegion.
 */

#define DEBUG_SWAP			0

static uint8_t buffer[PAGE_SIZE];

bool VirtMem::acquire() const {
	return Proc::request(pid,PLOCK_REGIONS) != NULL;
}

bool VirtMem::tryAquire() const {
	return Proc::tryRequest(pid,PLOCK_REGIONS) != NULL;
}

void VirtMem::release() const {
	Proc::release(Proc::getByPid(pid),PLOCK_REGIONS);
}

uintptr_t VirtMem::addPhys(uintptr_t *phys,size_t bCount,size_t align,bool writable) {
	ssize_t res;
	size_t pages = BYTES_2_PAGES(bCount);
	frameno_t *frames = (frameno_t*)Cache::alloc(sizeof(frameno_t) * pages);
	if(frames == NULL)
		return 0;

	/* if *phys is not set yet, we should allocate physical contiguous memory */
	Thread::addHeapAlloc(frames);
	if(*phys == 0) {
		if(align) {
			ssize_t first = PhysMem::allocateContiguous(pages,align / PAGE_SIZE);
			if(first < 0)
				goto error;
			for(size_t i = 0; i < pages; i++)
				frames[i] = first + i;
		}
		else {
			for(size_t i = 0; i < pages; i++)
				frames[i] = PhysMem::allocate(PhysMem::USR);
		}
	}
	/* otherwise use the specified one */
	else {
		for(size_t i = 0; i < pages; i++) {
			frames[i] = *phys / PAGE_SIZE;
			*phys += PAGE_SIZE;
		}
	}

	/* create region */
	VMRegion *vm;
	res = map(0,bCount,0,PROT_READ | (writable ? PROT_WRITE : 0),
			*phys ? (MAP_NOMAP | MAP_NOFREE) : MAP_NOMAP,NULL,0,&vm);
	if(res < 0) {
		if(!*phys)
			PhysMem::freeContiguous(frames[0],pages);
		goto error;
	}

	/* map memory */
	if(!acquire())
		goto errorRem;
	res = getPageDir()->map(vm->virt,frames,pages,writable ? PG_PRESENT | PG_WRITABLE : PG_PRESENT);
	if(res < 0)
		goto errorRel;
	if(*phys) {
		/* the page-tables are ours, the pages may be used by others, too */
		addOwn(res);
		addShared(pages);
	}
	else {
		/* its our own mem; store physical address for the caller */
		addOwn(res);
		*phys = frames[0] * PAGE_SIZE;
	}
	release();
	Thread::remHeapAlloc(frames);
	Cache::free(frames);
	return vm->virt;

errorRel:
	release();
errorRem:
	remove(vm);
error:
	Thread::remHeapAlloc(frames);
	Cache::free(frames);
	return 0;
}

int VirtMem::map(uintptr_t addr,size_t length,size_t loadCount,int prot,int flags,OpenFile *f,
                 off_t offset,VMRegion **vmreg) {
	int res;
	VMRegion *vm;
	Region *reg;
	ulong pgFlags = f != NULL ? PF_DEMANDLOAD : 0;
	ulong rflags = flags | prot;
	assert(length > 0 && length >= loadCount);

	/* for files: try to find another process with that file */
	if(f && (rflags & MAP_SHARED)) {
		VirtMem *virtmem = NULL;
		uintptr_t binVirt = getBinary(f,virtmem);
		if(binVirt != 0) {
			if(virtmem->join(binVirt,this,vmreg,rflags & MAP_FIXED ? addr : 0) == 0)
				return 0;
			/* if it failed, try to map it on our own. maybe we couldn't lock the other process */
		}
	}

	/* get the attributes of the region (depending on type) */
	if(!acquire()) {
		res = -ESRCH;
		goto error;
	}

	/* if it's a data-region.. */
	if((rflags & (PROT_WRITE | MAP_GROWABLE | MAP_STACK)) == (PROT_WRITE | MAP_GROWABLE)) {
		/* if we already have the real data-region */
		if(dataAddr && dataAddr < FREE_AREA_BEGIN) {
			/* using a fixed address is not allowed in this case */
			if(rflags & MAP_FIXED) {
				res = -EINVAL;
				goto error;
			}
			/* in every case, it can't be growable */
			rflags &= ~MAP_GROWABLE;
		}
		/* otherwise, it's either the dynlink-data-region or the real one. both is ok */
	}

	/* determine address */
	res = -EINVAL;
	if(rflags & MAP_FIXED) {
		/* ok, the user has choosen a place. check it */
		if(!regtree.available(addr,length))
			goto errProc;
	}
	else {
		/* find a suitable place */
		if(rflags & MAP_STACK)
			addr = findFreeStack(length,rflags);
		else
			addr = freemap.allocate(ROUND_PAGE_UP(length));
		if(addr == 0)
			goto errProc;
	}

	/* create region */
	res = -ENOMEM;
	reg = CREATE(Region,f,length,loadCount,offset,pgFlags,rflags);
	if(!reg)
		goto errProc;
	if(!reg->addTo(this))
		goto errReg;
	vm = regtree.add(reg,addr);
	if(vm == NULL)
		goto errAdd;

	/* map into process */
	if(~rflags & MAP_NOMAP) {
		ssize_t pts;
		uint mapFlags = 0;
		size_t pageCount = BYTES_2_PAGES(vm->reg->getByteCount());
		if(!(pgFlags & PF_DEMANDLOAD)) {
			mapFlags |= PG_PRESENT;
			if(rflags & PROT_EXEC)
				mapFlags |= PG_EXECUTABLE;
		}
		if(rflags & PROT_WRITE)
			mapFlags |= PG_WRITABLE;
		pts = getPageDir()->map(addr,NULL,pageCount,mapFlags);
		if(pts < 0)
			goto errMap;
		if(rflags & MAP_SHARED)
			addShared(pageCount);
		else
			addOwn(pageCount);
		addOwn(pts);
	}
	/* set data-region */
	if((rflags & (PROT_WRITE | MAP_GROWABLE | MAP_STACK)) == (PROT_WRITE | MAP_GROWABLE))
		dataAddr = addr;
	release();

	if(DISABLE_DEMLOAD || (rflags & MAP_POPULATE)) {
		/* ensure that we don't discard frames that we might need afterwards */
		Thread *t = Thread::getRunning();
		size_t oldres = t->getReservedFrmCnt();
		t->discardFrames();
		for(size_t i = 0; i < vm->reg->getPageCount(); i++)
			pagefault(vm->virt + i * PAGE_SIZE,false);
		t->reserveFrames(oldres);
	}
	*vmreg = vm;
	return 0;

errMap:
	regtree.remove(vm);
errAdd:
	reg->remFrom(this);
errReg:
	delete reg;
errProc:
	release();
error:
	return res;
}

int VirtMem::regctrl(uintptr_t addr,ulong flags) {
	size_t pgcount;
	int res = -EPERM;
	if(!acquire())
		return -ESRCH;

	VMRegion *vmreg = regtree.getByAddr(addr);
	if(vmreg == NULL) {
		release();
		return -ENXIO;
	}

	vmreg->reg->acquire();
	if(!vmreg || (vmreg->reg->getFlags() & (RF_NOFREE | RF_STACK | RF_TLS)))
		goto error;

	/* check if COW is enabled for a page */
	pgcount = BYTES_2_PAGES(vmreg->reg->getByteCount());
	for(size_t i = 0; i < pgcount; i++) {
		if(vmreg->reg->getPageFlags(i) & PF_COPYONWRITE)
			goto error;
	}

	/* change reg flags */
	vmreg->reg->setFlags((vmreg->reg->getFlags() & ~(RF_WRITABLE | RF_EXECUTABLE)) | flags);

	/* change mapping */
	for(auto mp = vmreg->reg->vmbegin(); mp != vmreg->reg->vmend(); ++mp) {
		/* the region may be mapped to a different virtual address */
		VMRegion *mpreg = (*mp)->regtree.getByReg(vmreg->reg);
		assert(mpreg != NULL);
		for(size_t i = 0; i < pgcount; i++) {
			/* determine flags; we can't always mark it present.. */
			uint mapFlags = PG_KEEPFRM;
			if(!(vmreg->reg->getPageFlags(i) & (PF_DEMANDLOAD | PF_SWAPPED)))
				mapFlags |= PG_PRESENT;
			if(flags & RF_EXECUTABLE)
				mapFlags |= PG_EXECUTABLE;
			if(flags & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			/* can't fail because of PG_KEEPFRM and because the page-table is always present */
			assert((*mp)->getPageDir()->map(mpreg->virt + i * PAGE_SIZE,NULL,1,mapFlags) == 0);
		}
	}
	res = 0;

error:
	vmreg->reg->release();
	release();
	return res;
}

void VirtMem::swapOut(pid_t pid,OpenFile *file,size_t count) {
	while(count > 0) {
		Region *reg = getLRURegion();
		if(reg == NULL)
			Util::panic("No pages to swap out");

		/* the list of processes may have changed in the meantime; we can't swap out in this case */
		while(count > 0) {
			/* get page to swap out */
			ssize_t index = getPgIdxForSwap(reg);
			if(index < 0)
				break;

			/* get VM-region of first process */
			VirtMem *vm = *reg->vmbegin();
			VMRegion *vmreg = vm->regtree.getByReg(reg);

			/* find swap-block */
			ulong block = SwapMap::alloc();
			assert(block != INVALID_BLOCK);

#if DEBUG_SWAP
			Log::get().writef("OUT: %d of region %x (block %d)\n",index,vmreg->reg,block);
			for(auto mp = reg->vmbegin(); mp != reg->vmend(); ++mp) {
				VMRegion *mpreg = (*mp)->regtree.getByReg(reg);
				Log::get().writef("\tProcess %d:%s -> page %p\n",(*mp)->getPid(),
						Proc::getByPid((*mp)->getPid())->getCommand(),mpreg->virt + index * PAGE_SIZE);
			}
			Log::get().writef("\n");
#endif

			/* get the frame first, because the page has to be present */
			frameno_t frameNo = vm->getPageDir()->getFrameNo(vmreg->virt + index * PAGE_SIZE);
			/* unmap the page in all processes and ensure that all CPUs have flushed their TLB */
			/* this way we know that nobody can still access the page; if someone tries, he will
			 * cause a page-fault and will wait until we release the region-mutex */
			setSwappedOut(reg,index);
			reg->setSwapBlock(index,block);
			SMP::ensureTLBFlushed();

			/* copy to a temporary buffer because we can't use the temp-area when switching threads */
			PageDir::copyFromFrame(frameNo,buffer);
			PhysMem::free(frameNo,PhysMem::USR);

			/* write out on disk */
			assert(file->seek(pid,block * PAGE_SIZE,SEEK_SET) >= 0);
			assert(file->write(pid,buffer,PAGE_SIZE) == PAGE_SIZE);

			count--;
		}
		reg->release();
	}
}

bool VirtMem::swapIn(pid_t pid,OpenFile *file,Thread *t,uintptr_t addr) {
	VMRegion *vmreg = t->getProc()->getVM()->regtree.getByAddr(addr);
	if(!vmreg)
		return false;

	addr &= ~(PAGE_SIZE - 1);
	size_t index = (addr - vmreg->virt) / PAGE_SIZE;

	/* not swapped anymore? so probably another process has already swapped it in */
	if(!(vmreg->reg->getPageFlags(index) & PF_SWAPPED))
		return false;

	ulong block = vmreg->reg->getSwapBlock(index);

#if DEBUG_SWAP
	Log::get().writef("IN: %d of region %x (block %d)\n",index,vmreg->reg,block);
	for(auto mp = vmreg->reg->vmbegin(); mp != vmreg->reg->vmend(); ++mp) {
		VMRegion *mpreg = (*mp)->regtree.getByReg(vmreg->reg);
		Log::get().writef("\tProcess %d:%s -> page %p\n",(*mp)->getPid(),
				Proc::getByPid((*mp)->getPid())->getCommand(),mpreg->virt + index * PAGE_SIZE);
	}
	Log::get().writef("\n");
#endif

	/* read into buffer (note that we can use the same for swap-in and swap-out because its both
	 * done by the swapper-thread) */
	assert(file->seek(pid,block * PAGE_SIZE,SEEK_SET) >= 0);
	assert(file->read(pid,buffer,PAGE_SIZE) == PAGE_SIZE);

	/* copy into a new frame */
	frameno_t frame = t->getFrame();
	PageDir::copyToFrame(frame,buffer);

	/* mark as not-swapped and map into all affected processes */
	setSwappedIn(vmreg->reg,index,frame);
	/* free swap-block */
	SwapMap::free(block);
	return true;
}

void VirtMem::setTimestamp(Thread *t,uint64_t timestamp) {
	if(!PhysMem::shouldSetRegTimestamp())
		return;

	/* ignore setting the timestamp if the process is currently locked; its not that critical and
	 * we can't wait for the mutex here because this is called from Thread::switchAway() and the wait
	 * for the mutex would call that as well */
	VirtMem *vm = t->getProc()->getVM();
	if(vm->tryAquire()) {
		if(t->getTLSRegion())
			t->getTLSRegion()->reg->setTimestamp(timestamp);
		for(size_t i = 0; i < STACK_REG_COUNT; i++) {
			if(t->getStackRegion(i))
				t->getStackRegion(i)->reg->setTimestamp(timestamp);
		}
		for(VMRegion *reg = vm->regtree.first(); reg != NULL; reg = reg->next) {
			if(!(reg->reg->getFlags() & (RF_TLS | RF_STACK)))
				reg->reg->setTimestamp(timestamp);
		}
		vm->release();
	}
}

float VirtMem::getMemUsage(size_t *pages) const {
	float rpages = 0;
	*pages = 0;
	if(acquire()) {
		for(VMRegion *vm = regtree.first(); vm != NULL; vm = vm->next) {
			size_t count = 0;
			size_t pageCount = BYTES_2_PAGES(vm->reg->getByteCount());
			for(size_t j = 0; j < pageCount; j++) {
				if(!(vm->reg->getPageFlags(j) & (PF_SWAPPED | PF_DEMANDLOAD | PF_COPYONWRITE)))
					count++;
			}
			*pages += pageCount;
			rpages += (float)count / vm->reg->refCount();
		}
		release();
	}
	return rpages;
}

bool VirtMem::getRegRange(VMRegion *vm,uintptr_t *start,uintptr_t *end,bool locked) const {
	bool res = false;
	if(!locked || acquire()) {
		if(start)
			*start = vm->virt;
		if(end)
			*end = vm->virt + vm->reg->getByteCount();
		if(locked)
			release();
		res = true;
	}
	return res;
}

uintptr_t VirtMem::getBinary(OpenFile *file,VirtMem *&binOwner) {
	uintptr_t res = 0;
	VMTree *tree = VMTree::reqTree();
	for(; tree != NULL; tree = tree->getNext()) {
		/* if we can't acquire the lock, don't consider this tree */
		if(tree->getVM()->tryAquire()) {
			for(VMRegion *vm = tree->first(); vm != NULL; vm = vm->next) {
				/* if its shareable and the binary fits, return region-number */
				if((vm->reg->getFlags() & RF_SHAREABLE) && vm->reg->getFile() &&
						vm->reg->getFile()->isSameFile(file)) {
					res = vm->virt;
					binOwner = tree->getVM();
					break;
				}
			}
			tree->getVM()->release();
		}
	}
	VMTree::relTree();
	return res;
}

bool VirtMem::pagefault(uintptr_t addr,bool write) {
	Thread *t = Thread::getRunning();
	bool res = false;
	VMRegion *reg;

	/* we can swap here; note that we don't need page-tables in this case, they're always present */
	if(!t->reserveFrames(1))
		return false;

	VirtMem *vm = t->getProc()->getVM();
	vm->acquire();
	reg = vm->regtree.getByAddr(addr);
	if(reg == NULL) {
		vm->release();
		t->discardFrames();
		return false;
	}
	res = vm->doPagefault(addr,reg,write);
	vm->release();
	t->discardFrames();
	return res;
}

bool VirtMem::doPagefault(uintptr_t addr,VMRegion *vm,bool write) {
	bool res = false;
	size_t page = (addr - vm->virt) / PAGE_SIZE;
	ulong flags;
	vm->reg->acquire();
	flags = vm->reg->getPageFlags(page);
	addr &= ~(PAGE_SIZE - 1);
	if(flags & PF_DEMANDLOAD) {
		res = demandLoad(vm,addr);
		if(res)
			vm->reg->setPageFlags(page,flags & ~PF_DEMANDLOAD);
	}
	else if(flags & PF_SWAPPED)
		res = PhysMem::swapIn(addr);
	else if(flags & PF_COPYONWRITE) {
		frameno_t frameNumber = getPageDir()->getFrameNo(addr);
		size_t frmCount = CopyOnWrite::pagefault(addr,frameNumber);
		addOwn(frmCount);
		addShared(-frmCount);
		vm->reg->setPageFlags(page,flags & ~PF_COPYONWRITE);
		res = true;
	}
	else {
		vassert(flags == 0,"Flags: %x",flags);
		/* its ok if its either a read-access or a write-access on a writeable region */
		/* note: this map happen if e.g. two threads of a process cause a page-fault simultanously.
		 * of course, only one thread gets the region-mutex and handles the page-fault. the other
		 * one will get the mutex afterwards and the flags will be zero, because the page-fault
		 * has already been handled. */
		res = !write || (vm->reg->getFlags() & RF_WRITABLE);
	}
	vm->reg->release();
	return res;
}

void VirtMem::removeAll(bool remStack) {
	if(acquire()) {
		for(VMRegion *vm = regtree.first(); vm != NULL; vm = vm->next) {
			if((!(vm->reg->getFlags() & RF_STACK) || remStack))
				doRemove(vm);
		}
		release();
	}
}

void VirtMem::remove(VMRegion *vm) {
	if(acquire()) {
		doRemove(vm);
		release();
	}
}

void VirtMem::sync(VMRegion *vm) const {
	OpenFile *file = vm->reg->getFile();
	if((vm->reg->getFlags() & RF_SHAREABLE) && (vm->reg->getFlags() & RF_WRITABLE) && file) {
		size_t pcount = BYTES_2_PAGES(vm->reg->getByteCount());
		/* this is more complicated because p might not be the current process. in this case we
		 * can't directly access the memory. */
		pid_t cur = Proc::getRunning();
		uint8_t *buf = NULL;
		if(cur != pid) {
			buf = (uint8_t*)Cache::alloc(PAGE_SIZE);
			if(buf == NULL)
				return;
			Thread::addHeapAlloc(buf);
		}

		for(size_t i = 0; i < pcount; i++) {
			size_t amount = i < pcount - 1 ? PAGE_SIZE : (vm->reg->getByteCount() - i * PAGE_SIZE);
			if(file->seek(pid,vm->reg->getOffset() + i * PAGE_SIZE,SEEK_SET) < 0)
				return;
			if(pid == cur)
				file->write(pid,(void*)(vm->virt + i * PAGE_SIZE),amount);
			else {
				/* we can't use the temp mapping during file->writeFile() because we might perform a
				 * context-switch in between. */
				frameno_t frame = getPageDir()->getFrameNo(vm->virt + i * PAGE_SIZE);
				uintptr_t addr = PageDir::getAccess(frame);
				memcpy(buf,(void*)addr,amount);
				PageDir::removeAccess();
				file->write(pid,buf,amount);
			}
		}

		if(cur != pid) {
			Thread::remHeapAlloc(buf);
			Cache::free(buf);
		}
	}
}

void VirtMem::doRemove(VMRegion *vm) {
	vm->reg->acquire();
	size_t pcount = BYTES_2_PAGES(vm->reg->getByteCount());
	assert(vm->reg->remFrom(this));
	if(vm->reg->refCount() == 0) {
		uintptr_t virt = vm->virt;
		/* first, write the content of the memory back to the file, if necessary */
		sync(vm);
		/* remove us from cow and unmap the pages (and free frames, if necessary) */
		for(size_t i = 0; i < pcount; i++) {
			ssize_t pts;
			bool freeFrame = !(vm->reg->getFlags() & RF_NOFREE);
			if(vm->reg->getPageFlags(i) & PF_COPYONWRITE) {
				bool foundOther;
				frameno_t frameNo = getPageDir()->getFrameNo(virt);
				/* we can free the frame if there is no other user */
				addShared(-CopyOnWrite::remove(frameNo,&foundOther));
				freeFrame = !foundOther;
			}
			if(vm->reg->getPageFlags(i) & PF_SWAPPED)
				addSwap(-1);
			pts = getPageDir()->unmap(virt,1,freeFrame);
			if(freeFrame && !(vm->reg->getPageFlags(i) & (PF_COPYONWRITE | PF_DEMANDLOAD))) {
				if(vm->reg->getFlags() & RF_SHAREABLE)
					addShared(-1);
				else
					addOwn(-1);
			}
			addOwn(-pts);
			virt += PAGE_SIZE;
		}
		/* store next free stack-address, if its a stack */
		if(vm->virt + vm->reg->getByteCount() > freeStackAddr && (vm->reg->getFlags() & RF_STACK))
			freeStackAddr = vm->virt + vm->reg->getByteCount();
		/* give the memory back to the free-area, if its in there */
		else if(vm->virt >= FREE_AREA_BEGIN)
			freemap.free(vm->virt,ROUND_PAGE_UP(vm->reg->getByteCount()));
		if(vm->virt == dataAddr)
			dataAddr = 0;
		/* now destroy region */
		vm->reg->release();
		/* do the remove BEFORE the free */
		regtree.remove(vm);
		delete vm->reg;
	}
	else {
		size_t sw;
		/* no free here, just unmap */
		ssize_t pts = getPageDir()->unmap(vm->virt,pcount,false);
		/* in this case its always a shared region because otherwise there wouldn't be other users */
		/* so we have to substract the present content-frames from the shared ones,
		 * and the ptables from ours */
		addShared(-vm->reg->pageCount(&sw));
		addOwn(-pts);
		addSwap(-sw);
		/* give the memory back to the free-area, if its in there */
		if(vm->virt >= FREE_AREA_BEGIN)
			freemap.free(vm->virt,ROUND_PAGE_UP(vm->reg->getByteCount()));
		vm->reg->release();
		regtree.remove(vm);
	}
}

int VirtMem::join(uintptr_t srcAddr,VirtMem *dst,VMRegion **nvm,uintptr_t dstAddr) {
	size_t sw,pageCount;
	ssize_t res;
	if(dst == this)
		return -EINVAL;

	if(!acquire())
		return -ESRCH;
	// better use tryAquire here to prevent a deadlock; this is of course suboptimal because the
	// caller would have to retry the operation in this case
	if(!dst->tryAquire()) {
		release();
		return -ESRCH;
	}

	VMRegion *vm = regtree.getByAddr(srcAddr);
	if(vm == NULL)
		goto errProc;

	vm->reg->acquire();
	assert(vm->reg->getFlags() & RF_SHAREABLE);

	if(dstAddr == 0)
		dstAddr = dst->freemap.allocate(ROUND_PAGE_UP(vm->reg->getByteCount()));
	else if(dst->regtree.getByAddr(dstAddr) != NULL)
		goto errReg;
	if(dstAddr == 0)
		goto errReg;
	*nvm = dst->regtree.add(vm->reg,dstAddr);
	if(*nvm == NULL)
		goto errReg;
	if(!vm->reg->addTo(dst))
		goto errAdd;

	/* shared, so content-frames to shared, ptables to own */
	pageCount = BYTES_2_PAGES(vm->reg->getByteCount());
	res = getPageDir()->clonePages(dst->getPageDir(),vm->virt,(*nvm)->virt,pageCount,true);
	if(res < 0)
		goto errRem;
	dst->addShared(vm->reg->pageCount(&sw));
	dst->addOwn(res);
	dst->addSwap(sw);
	vm->reg->release();
	dst->release();
	release();
	return 0;

errRem:
	vm->reg->remFrom(dst);
errAdd:
	dst->regtree.remove((*nvm));
errReg:
	vm->reg->release();
errProc:
	dst->release();
	release();
	return -ENOMEM;
}

int VirtMem::cloneAll(VirtMem *dst) {
	Thread *t = Thread::getRunning();
	VMRegion *vm,*nvm;
	Region *reg;
	size_t j;
	uintptr_t virt;
	assert(pid == t->getProc()->getPid());

	if(!acquire())
		return -ESRCH;
	if(!dst->tryAquire()) {
		release();
		return -ESRCH;
	}

	dst->swapped = 0;
	dst->sharedFrames = 0;
	dst->ownFrames = 0;
	dst->dataAddr = dataAddr;

	VMTree::addTree(dst,&dst->regtree);
	for(vm = regtree.first(); vm != NULL; vm = vm->next) {
		/* just clone the tls- and stack-region of the current thread */
		if((!(vm->reg->getFlags() & RF_STACK) || t->hasStackRegion(vm)) &&
				(!(vm->reg->getFlags() & RF_TLS) || t->getTLSRegion() == vm)) {
			vm->reg->acquire();
			/* TODO ?? better don't share the file; they may have to read in parallel */
			if(vm->reg->getFlags() & RF_SHAREABLE) {
				vm->reg->addTo(dst);
				reg = vm->reg;
			}
			else {
				reg = CLONE(Region,*vm->reg,dst);
				if(reg == NULL)
					goto errorRel;
			}

			nvm = dst->regtree.add(reg,vm->virt);
			if(nvm == NULL)
				goto errorReg;

			/* remove regions in the free area from the free-map */
			if(vm->virt >= FREE_AREA_BEGIN) {
				if(!dst->freemap.allocateAt(nvm->virt,ROUND_PAGE_UP(nvm->reg->getByteCount())))
					goto errorRem;
			}

			/* now copy the pages */
			size_t pageCount = BYTES_2_PAGES(nvm->reg->getByteCount());
			ssize_t res = getPageDir()->clonePages(dst->getPageDir(),vm->virt,nvm->virt,pageCount,
					vm->reg->getFlags() & RF_SHAREABLE);
			if(res < 0)
				goto errorFreeArea;
			dst->addOwn(res);

			/* update stats */
			if(vm->reg->getFlags() & RF_SHAREABLE) {
				size_t sw;
				dst->addShared(nvm->reg->pageCount(&sw));
				dst->addSwap(sw);
			}
			/* add frames to copy-on-write, if not shared */
			else {
				virt = nvm->virt;
				for(j = 0; j < pageCount; j++) {
					if(vm->reg->getPageFlags(j) & PF_SWAPPED)
						dst->addSwap(1);
					/* not when demand-load or swapping is outstanding since we've not loaded it
					 * from disk yet */
					else if(!(vm->reg->getPageFlags(j) & (PF_DEMANDLOAD | PF_SWAPPED))) {
						bool marked = false;
						frameno_t frameNo = getPageDir()->getFrameNo(virt);
						/* if not already done, mark as cow for parent */
						if(!(vm->reg->getPageFlags(j) & PF_COPYONWRITE)) {
							if(!CopyOnWrite::add(frameNo))
								goto errorPages;
							vm->reg->setPageFlags(j,vm->reg->getPageFlags(j) | PF_COPYONWRITE);
							addShared(1);
							addOwn(-1);
							marked = true;
						}
						/* do it always for the child */
						nvm->reg->setPageFlags(j,nvm->reg->getPageFlags(j) | PF_COPYONWRITE);
						dst->addShared(1);
						if(!CopyOnWrite::add(frameNo)) {
							if(marked) {
								bool other;
								CopyOnWrite::remove(frameNo,&other);
								if(!other)
									vm->reg->setPageFlags(j,vm->reg->getPageFlags(j) & ~PF_COPYONWRITE);
								addShared(-1);
								addOwn(1);
							}
							goto errorPages;
						}
					}
					virt += PAGE_SIZE;
				}
			}
			vm->reg->release();
		}
	}
	dst->release();
	release();
	return 0;

errorPages:
	/* undo the stats-change for this and remove the frames from copy-on-write */
	for(; j-- > 0; ) {
		virt -= PAGE_SIZE;
		if(!(vm->reg->getPageFlags(j) & (PF_DEMANDLOAD | PF_SWAPPED))) {
			bool other;
			frameno_t frameNo = getPageDir()->getFrameNo(virt);
			CopyOnWrite::remove(frameNo,&other);
			if(vm->reg->getPageFlags(j) & PF_COPYONWRITE) {
				CopyOnWrite::remove(frameNo,&other);
				/* if there are no other users of this frame, it's our own and we don't have to
				 * mark it as copy-on-write anymore */
				if(!other)
					vm->reg->setPageFlags(j,vm->reg->getPageFlags(j) & ~PF_COPYONWRITE);
				addShared(-1);
				addOwn(1);
			}
		}
	}
errorFreeArea:
	if(vm->virt >= FREE_AREA_BEGIN)
		dst->freemap.free(nvm->virt,ROUND_PAGE_UP(nvm->reg->getByteCount()));
errorRem:
	dst->regtree.remove(nvm);
errorReg:
	if(vm->reg->getFlags() & RF_SHAREABLE)
		vm->reg->remFrom(dst);
	else
		delete reg;
errorRel:
	vm->reg->release();
	dst->release();
	release();
	/* Note that vmm_remove() will undo the cow just for the child; but thats ok since
	 * the parent will get its frame back as soon as it writes to it */
	/* Note also that we don't restore the old frame-counts; for dst it makes no sense because
	 * we'll destroy the process anyway. for src we can't do it because the cow-entries still
	 * exists and therefore its correct. */
	Proc::removeRegions(dst->pid,true);
	/* no need to free regs, since vmm_remove has already done it */
	return -ENOMEM;
}

int VirtMem::growStackTo(VMRegion *vm,uintptr_t addr) {
	int res = -ENOMEM;
	if(acquire()) {
		ssize_t newPages = 0;
		addr &= ~(PAGE_SIZE - 1);
		/* report failure if its outside (upper / lower) of the region */
		/* note that we assume here that if a thread has multiple stack-regions, they grow towards
		 * each other */
		if(vm->reg->getFlags() & RF_GROWS_DOWN) {
			if(addr < vm->virt + ROUND_PAGE_UP(vm->reg->getByteCount()) && addr < vm->virt) {
				res = 0;
				newPages = (vm->virt - addr) / PAGE_SIZE;
			}
		}
		else {
			if(addr >= vm->virt && addr >= vm->virt + ROUND_PAGE_UP(vm->reg->getByteCount())) {
				res = 0;
				newPages = ROUND_PAGE_UP(addr -
						(vm->virt + ROUND_PAGE_UP(vm->reg->getByteCount()) - 1)) / PAGE_SIZE;
			}
		}

		if(res == 0) {
			/* if its too much, try the next one; if there is none that fits, report failure */
			if(BYTES_2_PAGES(vm->reg->getByteCount()) + newPages >= MAX_STACK_PAGES - 1)
				res = -ENOMEM;
			/* new pages necessary? */
			else if(newPages > 0) {
				Thread *t = Thread::getRunning();
				if(!t->reserveFrames(newPages)) {
					release();
					return -ENOMEM;
				}
				res = doGrow(vm,newPages) != 0 ? 0 : -ENOMEM;
				t->discardFrames();
			}
		}
		release();
	}
	return res;
}

size_t VirtMem::grow(uintptr_t addr,ssize_t amount) {
	if(!acquire())
		return 0;
	size_t res = 0;
	VMRegion *vm = regtree.getByAddr(addr);
	if(vm)
		res = doGrow(vm,amount);
	release();
	return res;
}

size_t VirtMem::doGrow(VMRegion *vm,ssize_t amount) {
	ssize_t res = -ENOMEM;
	vm->reg->acquire();
	assert((vm->reg->getFlags() & RF_GROWABLE) && !(vm->reg->getFlags() & RF_SHAREABLE));

	/* check whether we've reached the max stack-pages */
	if((vm->reg->getFlags() & RF_STACK) &&
			BYTES_2_PAGES(vm->reg->getByteCount()) + amount >= MAX_STACK_PAGES - 1) {
		vm->reg->release();
		return 0;
	}

	/* resize region */
	uintptr_t oldVirt = vm->virt;
	size_t oldSize = vm->reg->getByteCount();
	if(amount != 0) {
		ssize_t pts;
		/* check whether the space is free */
		if(amount > 0) {
			if(vm->reg->getFlags() & RF_GROWS_DOWN) {
				if(isOccupied(oldVirt - amount * PAGE_SIZE,oldVirt)) {
					vm->reg->release();
					return 0;
				}
			}
			else {
				uintptr_t end = oldVirt + ROUND_PAGE_UP(vm->reg->getByteCount());
				if(isOccupied(end,end + amount * PAGE_SIZE)) {
					vm->reg->release();
					return 0;
				}
			}
		}
		if((res = vm->reg->grow(amount)) < 0) {
			vm->reg->release();
			return 0;
		}

		/* map / unmap pages */
		uintptr_t virt;
		if(amount > 0) {
			uint mapFlags = PG_PRESENT;
			if(vm->reg->getFlags() & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			/* if it grows down, pages are added before the existing */
			if(vm->reg->getFlags() & RF_GROWS_DOWN) {
				vm->virt -= amount * PAGE_SIZE;
				virt = vm->virt;
			}
			else
				virt = vm->virt + ROUND_PAGE_UP(oldSize);
			pts = getPageDir()->map(virt,NULL,amount,mapFlags);
			if(pts < 0) {
				if(vm->reg->getFlags() & RF_GROWS_DOWN)
					vm->virt += amount * PAGE_SIZE;
				vm->reg->release();
				return 0;
			}
			addOwn(pts + amount);
			/* remove it from the free area (used by the dynlinker for its data-region only) */
			/* note that we assume here that we get the location at the end of the data-region */
			if(vm->virt >= FREE_AREA_BEGIN)
				freemap.allocate(amount * PAGE_SIZE);
		}
		else {
			if(vm->reg->getFlags() & RF_GROWS_DOWN) {
				virt = vm->virt;
				vm->virt += -amount * PAGE_SIZE;
			}
			else
				virt = vm->virt + ROUND_PAGE_UP(vm->reg->getByteCount());
			/* give it back to the free area */
			if(vm->virt >= FREE_AREA_BEGIN)
				freemap.free(virt,-amount * PAGE_SIZE);
			pts = getPageDir()->unmap(virt,-amount,true);
			addOwn(-(pts - amount));
			addSwap(-res);
		}
	}

	res = (vm->reg->getFlags() & RF_GROWS_DOWN) ? oldVirt : oldVirt + ROUND_PAGE_UP(oldSize);
	vm->reg->release();
	return res;
}

const char *VirtMem::getRegName(const VMRegion *vm) const {
	const char *name = "";
	if(vm->virt == dataAddr)
		name = "data";
	else if(vm->reg->getFlags() & RF_STACK)
		name = "stack";
	else if(vm->reg->getFlags() & RF_TLS)
		name = "tls";
	else if(vm->reg->getFlags() & RF_NOFREE)
		name = "phys";
	else if(vm->reg->getFile())
		name = vm->reg->getFile()->getPath();
	else {
		Proc *p = Proc::getByPid(pid);
		if(p->getEntryPoint() >= vm->virt && p->getEntryPoint() < vm->virt + vm->reg->getByteCount())
			name = p->getCommand();
		else
			name = "???";
	}
	return name;
}

void VirtMem::printMaps(OStream &os) const {
	if(acquire()) {
		for(VMRegion *vm = regtree.first(); vm != NULL; vm = vm->next) {
			vm->reg->acquire();
			os.writef("%-24s %p - %p (%5zuK) %c%c%c%c",getRegName(vm),vm->virt,
					vm->virt + vm->reg->getByteCount() - 1,vm->reg->getByteCount() / K,
					(vm->reg->getFlags() & RF_WRITABLE) ? 'w' : '-',
					(vm->reg->getFlags() & RF_EXECUTABLE) ? 'x' : '-',
					(vm->reg->getFlags() & RF_GROWABLE) ? 'g' : '-',
					(vm->reg->getFlags() & RF_SHAREABLE) ? 's' : '-');
			vm->reg->release();
			os.writef("\n");
		}
		release();
	}
}

void VirtMem::printShort(OStream &os,const char *prefix) const {
	if(acquire()) {
		for(VMRegion *vm = regtree.first(); vm != NULL; vm = vm->next) {
			os.writef("%s%-24s %p - %p (%5zuK) ",prefix,getRegName(vm),vm->virt,
					vm->virt + vm->reg->getByteCount() - 1,vm->reg->getByteCount() / K);
			vm->reg->printFlags(os);
			os.writef("\n");
		}
		release();
	}
}

void VirtMem::printRegions(OStream &os) const {
	if(acquire()) {
		size_t c = 0;
		for(VMRegion *vm = regtree.first(); vm != NULL; vm = vm->next) {
			if(c)
				os.writef("\n");
			vm->reg->acquire();
			os.writef("VMRegion (%p .. %p):\n",vm->virt,vm->virt + vm->reg->getByteCount() - 1);
			vm->reg->print(os,vm->virt);
			vm->reg->release();
			c++;
		}
		if(c == 0)
			os.writef("- no regions -\n");
		release();
	}
}

void VirtMem::print(OStream &os) const {
	freemap.print(os);
	os.writef("Regions:\n");
	os.writef("\tDataRegion: %p\n",dataAddr);
	os.writef("\tFreeStack: %p\n",freeStackAddr);
	printShort(os,"\t");
}

bool VirtMem::demandLoad(VMRegion *vm,uintptr_t addr) {
	bool res = false;

	/* calculate the number of bytes to load and zero */
	size_t loadCount = 0, zeroCount;
	if(addr - vm->virt < vm->reg->getLoadCount())
		loadCount = MIN(PAGE_SIZE,vm->reg->getLoadCount() - (addr - vm->virt));
	else
		res = true;
	zeroCount = MIN(PAGE_SIZE,vm->reg->getByteCount() - (addr - vm->virt)) - loadCount;

	/* load from file */
	if(loadCount)
		res = loadFromFile(vm,addr,loadCount);

	/* zero the rest, if necessary */
	if(res && zeroCount) {
		/* do the memclear before the mapping to ensure that it's ready when the first CPU sees it */
		frameno_t frame = loadCount ? Proc::getCurPageDir()->getFrameNo(addr)
									: Thread::getRunning()->getFrame();
		uintptr_t frameAddr = PageDir::getAccess(frame);
		memclear((void*)(frameAddr + loadCount),zeroCount);
		PageDir::removeAccess();
		/* if the pages weren't present so far, map them into every process that has this region */
		if(!loadCount) {
			uint mapFlags = PG_PRESENT;
			if(vm->reg->getFlags() & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			for(auto mp = vm->reg->vmbegin(); mp != vm->reg->vmend(); ++mp) {
				/* the region may be mapped to a different virtual address */
				VMRegion *mpreg = (*mp)->regtree.getByReg(vm->reg);
				/* can't fail */
				assert((*mp)->getPageDir()->map(mpreg->virt + (addr - vm->virt),&frame,1,mapFlags) == 0);
				(*mp)->addShared(1);
			}
		}
	}
	return res;
}

bool VirtMem::loadFromFile(VMRegion *vm,uintptr_t addr,size_t loadCount) {
	uint mapFlags;
	frameno_t frame;
	void *tempBuf;
	/* note that we currently ignore that the file might have changed in the meantime */
	int err;
	if((err = vm->reg->getFile()->seek(pid,vm->reg->getOffset() + (addr - vm->virt),SEEK_SET)) < 0)
		goto error;

	/* first read into a temp-buffer because we can't mark the page as present until
	 * its read from disk. and we can't use a temporary mapping when switching
	 * threads. */
	tempBuf = Cache::alloc(PAGE_SIZE);
	if(tempBuf == NULL) {
		err = -ENOMEM;
		goto error;
	}
	Thread::addHeapAlloc(tempBuf);
	err = vm->reg->getFile()->read(pid,tempBuf,loadCount);
	if(err != (ssize_t)loadCount) {
		if(err >= 0)
			err = -ENOMEM;
		goto errorFree;
	}

	/* copy into frame */
	frame = PageDir::demandLoad(tempBuf,loadCount,vm->reg->getFlags());

	/* free resources not needed anymore */
	Thread::remHeapAlloc(tempBuf);
	Cache::free(tempBuf);

	/* map into all pagedirs */
	mapFlags = PG_PRESENT;
	if(vm->reg->getFlags() & RF_WRITABLE)
		mapFlags |= PG_WRITABLE;
	if(vm->reg->getFlags() & RF_EXECUTABLE)
		mapFlags |= PG_EXECUTABLE;
	for(auto mp = vm->reg->vmbegin(); mp != vm->reg->vmend(); ++mp) {
		/* the region may be mapped to a different virtual address */
		VMRegion *mpreg = (*mp)->regtree.getByReg(vm->reg);
		/* can't fail */
		assert((*mp)->getPageDir()->map(mpreg->virt + (addr - vm->virt),&frame,1,mapFlags) == 0);
		if(vm->reg->getFlags() & RF_SHAREABLE)
			(*mp)->addShared(1);
		else
			(*mp)->addOwn(1);
	}
	return true;

errorFree:
	Thread::remHeapAlloc(tempBuf);
	Cache::free(tempBuf);
error:
	Log::get().writef("Demandload page @ %p for proc %s: %s (%d)\n",addr,
			Proc::getByPid(pid)->getCommand(),strerror(-err),err);
	return false;
}

Region *VirtMem::getLRURegion() {
	Region *lru = NULL;
	time_t ts = UINT_MAX;
	VMTree *tree = VMTree::reqTree();
	for(; tree != NULL; tree = tree->getNext()) {
		/* the disk-driver is not swappable */
		if(tree->getVM()->pid == DISK_PID)
			continue;

		/* same as below; we have to try to acquire the mutex, otherwise we risk a deadlock */
		if(tree->getVM()->tryAquire()) {
			for(VMRegion *vm = tree->first(); vm != NULL; vm = vm->next) {
				size_t count = 0;
				if((vm->reg->getFlags() & RF_NOFREE) || vm->reg->getTimestamp() >= ts)
					continue;

				/* we can't block here because otherwise we risk a deadlock. suppose that fs has to
				 * swap out to get more memory. if we want to demand-load something before this operation
				 * is finished and lock the region for that, the swapper will find this region at this
				 * place locked. so we have to skip it in this case to be able to continue. */
				if(vm->reg->tryAquire()) {
					size_t pages = BYTES_2_PAGES(vm->reg->getByteCount());
					for(size_t j = 0; j < pages; j++) {
						if(!(vm->reg->getPageFlags(j) & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD))) {
							count++;
							break;
						}
					}
					if(count > 0) {
						if(lru)
							lru->release();
						ts = vm->reg->getTimestamp();
						lru = vm->reg;
					}
					else
						vm->reg->release();
				}
			}
			tree->getVM()->release();
		}
	}
	VMTree::relTree();
	return lru;
}

ssize_t VirtMem::getPgIdxForSwap(const Region *reg) {
	size_t pages = BYTES_2_PAGES(reg->getByteCount());
	size_t count = 0;
	for(size_t i = 0; i < pages; i++) {
		if(!(reg->getPageFlags(i) & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD)))
			count++;
	}
	if(count > 0) {
		size_t index = Util::rand() % count;
		for(size_t i = 0; i < pages; i++) {
			if(!(reg->getPageFlags(i) & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD))) {
				if(index-- == 0)
					return i;
			}
		}
	}
	return -1;
}

void VirtMem::setSwappedOut(Region *reg,size_t index) {
	uintptr_t offset = index * PAGE_SIZE;
	reg->setPageFlags(index,reg->getPageFlags(index) | PF_SWAPPED);
	for(auto mp = reg->vmbegin(); mp != reg->vmend(); ++mp) {
		/* the region may be mapped to a different virtual address */
		VMRegion *mpreg = (*mp)->regtree.getByReg(reg);
		/* can't fail */
		assert((*mp)->getPageDir()->map(mpreg->virt + offset,NULL,1,0) == 0);
		if(reg->getFlags() & RF_SHAREABLE)
			(*mp)->addShared(-1);
		else
			(*mp)->addOwn(-1);
		(*mp)->addSwap(1);
	}
}

void VirtMem::setSwappedIn(Region *reg,size_t index,frameno_t frameNo) {
	uintptr_t offset = index * PAGE_SIZE;
	uint flags = PG_PRESENT;
	if(reg->getFlags() & RF_WRITABLE)
		flags |= PG_WRITABLE;
	if(reg->getFlags() & RF_EXECUTABLE)
		flags |= PG_EXECUTABLE;
	reg->setPageFlags(index,reg->getPageFlags(index) & ~PF_SWAPPED);
	reg->setSwapBlock(index,0);
	for(auto mp = reg->vmbegin(); mp != reg->vmend(); ++mp) {
		/* the region may be mapped to a different virtual address */
		VMRegion *mpreg = (*mp)->regtree.getByReg(reg);
		/* can't fail */
		assert((*mp)->getPageDir()->map(mpreg->virt + offset,&frameNo,1,flags) == 0);
		if(reg->getFlags() & RF_SHAREABLE)
			(*mp)->addShared(1);
		else
			(*mp)->addOwn(1);
		(*mp)->addSwap(-1);
	}
}

uintptr_t VirtMem::findFreeStack(size_t byteCount,A_UNUSED ulong rflags) {
	/* leave a gap between the stacks as a guard */
	if(byteCount > (MAX_STACK_PAGES - 1) * PAGE_SIZE)
		return 0;
#if STACK_AREA_GROWS_DOWN
	/* find end address */
	VMRegion *dataReg = regtree.getByAddr(dataAddr);
	uintptr_t addr,end;
	if(dataReg)
		end = dataReg->virt + ROUND_PAGE_UP(dataReg->reg->getByteCount());
	else
		end = getFirstUsableAddr();
	/* determine start address */
	if(freeStackAddr != 0)
		addr = freeStackAddr;
	else
		addr = STACK_AREA_END;
	size_t size = ROUND_PAGE_UP(byteCount);
	for(; addr > end; addr -= MAX_STACK_PAGES * PAGE_SIZE) {
		uintptr_t start = addr - size;
		/* in this case it is not necessary to use vmm_isOccupied() because all stack-regions
		 * have a fixed max-size and there is no other region that can be in that area. so its
		 * enough to test whether in this area is a region or not */
		if(regtree.getByAddr(start) == NULL) {
			freeStackAddr = addr - MAX_STACK_PAGES * PAGE_SIZE;
			return start;
		}
	}
#else
	for(uintptr_t addr = STACK_AREA_BEGIN; addr < STACK_AREA_END; addr += MAX_STACK_PAGES * PAGE_SIZE) {
		if(isOccupied(addr,addr + (MAX_STACK_PAGES - 1) * PAGE_SIZE) == NULL) {
			if(rflags & RF_GROWS_DOWN)
				return addr + (MAX_STACK_PAGES - 1) * PAGE_SIZE - ROUND_PAGE_UP(byteCount);
			return addr;
		}
	}
#endif
	return 0;
}

VMRegion *VirtMem::isOccupied(uintptr_t start,uintptr_t end) const {
	for(VMRegion *vm = regtree.first(); vm != NULL; vm = vm->next) {
		uintptr_t rstart = vm->virt;
		uintptr_t rend = vm->virt + ROUND_PAGE_UP(vm->reg->getByteCount());
		if(OVERLAPS(rstart,rend,start,end))
			return vm;
	}
	return NULL;
}

uintptr_t VirtMem::getFirstUsableAddr() const {
	uintptr_t addr = 0;
	for(VMRegion *vm = regtree.first(); vm != NULL; vm = vm->next) {
		if(!(vm->reg->getFlags() & RF_STACK) && vm->virt < FREE_AREA_BEGIN &&
				vm->virt + vm->reg->getByteCount() > addr) {
			addr = vm->virt + vm->reg->getByteCount();
		}
	}
	return ROUND_PAGE_UP(addr);
}
