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

#include <sys/common.h>
#include <sys/mem/region.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/copyonwrite.h>
#include <sys/mem/cache.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/shfiles.h>
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
#include <string.h>
#include <assert.h>

/**
 * The vmm-module manages the user-part of a process's virtual addressspace. That means it
 * manages the regions that are used by the process (as an instance of VMRegion) and decides
 * where the regions are placed. So it binds a region to a virtual address via VMRegion.
 */

#define DEBUG_SWAP			0

static uint8_t buffer[PAGE_SIZE];

void VirtMem::acquire() const {
	proc->lock(PLOCK_PROG);
}

bool VirtMem::tryAquire() const {
	return proc->tryLock(PLOCK_PROG);
}

void VirtMem::release() const {
	proc->unlock(PLOCK_PROG);
}

uintptr_t VirtMem::mapphys(uintptr_t *phys,size_t bCount,size_t align,int flags) {
	ssize_t res;
	size_t pages = BYTES_2_PAGES(bCount);
	frameno_t firstFrame = -1;

	/* allocate physical contiguous memory */
	if(!(flags & MAP_PHYS_MAP)) {
		if(align) {
			ssize_t first = PhysMem::allocateContiguous(pages,align / PAGE_SIZE);
			if(first < 0)
				return first;
			firstFrame = first;
		}
	}
	/* otherwise use the specified one */
	else
		firstFrame = *phys / PAGE_SIZE;

	/* create region */
	VMRegion *vm;
	/* for specifically requested physical memory, don't free it and don't swap it out */
	res = map(0,bCount,0,PROT_READ | PROT_WRITE,
			(flags & MAP_PHYS_MAP) ? (MAP_NOMAP | MAP_NOFREE | MAP_LOCKED) : MAP_NOMAP,NULL,0,&vm);
	if(res < 0) {
		if(!(flags & MAP_PHYS_MAP))
			PhysMem::freeContiguous(firstFrame,pages);
		return res;
	}

	acquire();

	/* map memory */
	uint mflags = PG_PRESENT | PG_WRITABLE;
	ulong pts;
	if(firstFrame != (frameno_t)-1) {
		PageTables::RangeAllocator alloc(firstFrame);
		res = getPageDir()->map(vm->virt(),pages,alloc,mflags);
		pts = alloc.pageTables();
	}
	else {
		PageTables::UAllocator alloc;
		res = getPageDir()->map(vm->virt(),pages,alloc,mflags);
		pts = alloc.pageTables();
	}

	if(res < 0)
		goto errorRel;
	if(flags & MAP_PHYS_MAP) {
		/* the page-tables are ours, the pages may be used by others, too */
		addOwn(pts);
		addShared(pages);
	}
	else {
		/* its our own mem; store physical address for the caller */
		addOwn(pts + pages);
		*phys = firstFrame * PAGE_SIZE;
	}
	release();
	return vm->virt();

errorRel:
	release();
	unmap(vm);
	return 0;
}

int VirtMem::map(uintptr_t *addr,size_t length,size_t loadCount,int prot,int flags,OpenFile *f,
                 off_t offset,VMRegion **vmreg) {
	int res;
	VMRegion *vm;
	Region *reg;
	ulong rflags = flags | prot;
	uintptr_t virt = addr ? *addr : 0;
	Thread *t = Thread::getRunning();
	size_t oldSh,oldOwn;
	size_t pageCount = BYTES_2_PAGES(length);
	PageTables::UAllocator alloc;
	assert(length > 0 && length >= loadCount);

	/* for files: try to find another process with that file */
	if(f && (rflags & MAP_SHARED)) {
		pid_t pid;
		uintptr_t shaddr;
		if(ShFiles::get(f,&shaddr,&pid)) {
			Proc *owner = Proc::getRef(pid);
			res = -ESRCH;
			if(owner) {
				res = owner->getVM()->join(shaddr,this,vmreg,addr,rflags);
				Proc::relRef(owner);
				if(res != -ESRCH && res != -ENXIO)
					return res;
			}

			/* if it failed because it's gone or we couldn't get the lock, try to map it on our own */
			/* but not if it's writable. in this case it HAVE TO be the same frames. so the user
			 * should retry that in this case */
			if(res < 0 && (rflags & PROT_WRITE))
				return -EBUSY;
		}
	}

	acquire();

	/* should we populate it but fail if there is not enough mem? */
	if((rflags & (MAP_POPULATE | MAP_NOSWAP)) == (MAP_POPULATE | MAP_NOSWAP)) {
		if(!t->reserveFrames(pageCount,false)) {
			res = -ENOMEM;
			goto errProc;
		}
	}

	/* if it's a data-region.. */
	if((rflags & (PROT_WRITE | MAP_GROWABLE | MAP_STACK)) == (PROT_WRITE | MAP_GROWABLE)) {
		/* if we already have the real data-region */
		if(dataAddr && dataAddr < FREE_AREA_BEGIN) {
			/* using a fixed address is not allowed in this case */
			if(rflags & MAP_FIXED) {
				res = -EINVAL;
				goto errProc;
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
		if(!regtree.available(virt,length))
			goto errProc;
	}
	else {
		/* find a suitable place */
		if(rflags & MAP_STACK)
			virt = findFreeStack(length,rflags);
		else
			virt = freemap.allocate(ROUND_PAGE_UP(length));
		if(virt == 0)
			goto errProc;
	}

	/* create region */
	res = -ENOMEM;
	reg = createObj<Region>(f,length,loadCount,offset,(rflags & MAP_NOMAP) ? 0 : PF_DEMANDLOAD,rflags);
	if(!reg)
		goto errProc;
	if(!reg->addTo(this))
		goto errReg;
	reg->acquire();
	vm = regtree.add(reg,virt);
	if(vm == NULL)
		goto errAdd;

	oldSh = getSharedFrames();
	oldOwn = getOwnFrames();

	/* map into process */
	if(~rflags & MAP_NOMAP) {
		res = getPageDir()->map(virt,pageCount,alloc,0);
		if(res < 0)
			goto errMap;
		addOwn(alloc.pageTables());
	}

	if(rflags & MAP_POPULATE) {
		assert(~rflags & MAP_NOMAP);
		res = populatePages(vm,pageCount);
		if(res < 0)
			goto errPf;
	}

	if((rflags & MAP_SHARED) && (res = ShFiles::add(vm,proc->getPid())) < 0)
		goto errPf;

	/* set data-region */
	if((rflags & (PROT_WRITE | MAP_GROWABLE | MAP_STACK)) == (PROT_WRITE | MAP_GROWABLE))
		dataAddr = *addr;

	if(addr)
		*addr = virt;
	*vmreg = vm;
	reg->release();
	release();
	return 0;

errPf:
	addShared(oldSh - getSharedFrames());
	addOwn(oldOwn - getOwnFrames());
	getPageDir()->unmap(*addr,pageCount,alloc);
errMap:
	regtree.remove(vm);
errAdd:
	reg->remFrom(this);
	reg->release();
errReg:
	delete reg;
errProc:
	t->discardFrames();
	release();
	return res;
}

int VirtMem::protect(uintptr_t addr,ulong flags) {
	size_t pgcount;
	int res = -EPERM;
	PageTables::NoAllocator alloc;
	acquire();
	VMRegion *vmreg = regtree.getByAddr(addr);
	if(vmreg == NULL) {
		release();
		return -ENXIO;
	}

	vmreg->reg->acquire();
	if(!vmreg || (vmreg->reg->getFlags() & (RF_NOFREE | RF_STACK)))
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
			uint mapFlags = 0;
			if(!(vmreg->reg->getPageFlags(i) & (PF_DEMANDLOAD | PF_SWAPPED)))
				mapFlags |= PG_PRESENT;
			if(flags & RF_EXECUTABLE)
				mapFlags |= PG_EXECUTABLE;
			if(flags & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			/* can't fail because of NoAllocator and because the page-table is always present */
			sassert((*mp)->getPageDir()->map(mpreg->virt() + i * PAGE_SIZE,1,alloc,mapFlags) == 0);
		}
	}
	res = 0;

error:
	vmreg->reg->release();
	release();
	return res;
}

int VirtMem::lock(uintptr_t addr,int flags) {
	acquire();
	VMRegion *vm = regtree.getByAddr(addr);
	if(vm == NULL) {
		release();
		return -ENXIO;
	}
	int res = lockRegion(vm,flags);
	release();
	return res;
}

int VirtMem::lockall() {
	int res = 0;
	acquire();
	for(auto vm = regtree.begin(); vm != regtree.end(); ++vm) {
		res = lockRegion(&*vm,MAP_NOSWAP);
		if(res < 0)
			break;
	}
	release();
	return res;
}

int VirtMem::lockRegion(VMRegion *vm,int flags) {
	Thread *t = Thread::getRunning();
	int res = 0;
	vm->reg->acquire();
	size_t pageCount = BYTES_2_PAGES(vm->reg->getByteCount());

	/* already locked? */
	if(vm->reg->getFlags() & RF_LOCKED)
		goto error;

	/* should we populate it but fail if there is not enough mem? */
	if(flags & MAP_NOSWAP) {
		size_t swCount;
		size_t presentCount = vm->reg->pageCount(&swCount);
		if(!t->reserveFrames(pageCount - presentCount,false)) {
			res = -ENOMEM;
			goto error;
		}
	}

	/* fault-in all pages that are not present yet */
	res = populatePages(vm,pageCount);
	if(res < 0)
		goto error;

	vm->reg->setFlags(vm->reg->getFlags() | RF_LOCKED);
error:
	vm->reg->release();
	return res;
}

int VirtMem::populatePages(VMRegion *vm,size_t count) {
	bool write = !!(vm->reg->getFlags() & PROT_WRITE);
	for(size_t i = 0; i < count; i++) {
		if(vm->reg->getPageFlags(i) & (PF_DEMANDLOAD | PF_COPYONWRITE | PF_SWAPPED)) {
			int res;
			if((res = doPagefault(vm->virt() + i * PAGE_SIZE,vm,write)) != 0)
				return res;
		}
	}
	return 0;
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
				Log::get().writef("\tProcess %d:%s -> page %p\n",(*mp)->getProc()->getPid(),
						(*mp)->getProc()->getProgram(),mpreg->virt() + index * PAGE_SIZE);
			}
			Log::get().writef("\n");
#endif

			/* get the frame first, because the page has to be present */
			frameno_t frameNo = vm->getPageDir()->getFrameNo(vmreg->virt() + index * PAGE_SIZE);
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
			sassert(file->seek(pid,block * PAGE_SIZE,SEEK_SET) >= 0);
			sassert(file->write(pid,buffer,PAGE_SIZE) == PAGE_SIZE);

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
	size_t index = (addr - vmreg->virt()) / PAGE_SIZE;

	/* not swapped anymore? so probably another process has already swapped it in */
	if(!(vmreg->reg->getPageFlags(index) & PF_SWAPPED))
		return false;

	ulong block = vmreg->reg->getSwapBlock(index);

#if DEBUG_SWAP
	Log::get().writef("IN: %d of region %x (block %d)\n",index,vmreg->reg,block);
	for(auto mp = vmreg->reg->vmbegin(); mp != vmreg->reg->vmend(); ++mp) {
		VMRegion *mpreg = (*mp)->regtree.getByReg(vmreg->reg);
		Log::get().writef("\tProcess %d:%s -> page %p\n",(*mp)->getProc()->getPid(),
				(*mp)->getProc()->getProgram(),mpreg->virt() + index * PAGE_SIZE);
	}
	Log::get().writef("\n");
#endif

	/* read into buffer (note that we can use the same for swap-in and swap-out because its both
	 * done by the swapper-thread) */
	sassert(file->seek(pid,block * PAGE_SIZE,SEEK_SET) >= 0);
	sassert(file->read(pid,buffer,PAGE_SIZE) == PAGE_SIZE);

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
	/* ignore setting the timestamp if the process is currently locked; its not that critical and
	 * we can't wait for the mutex here because this is called from Thread::switchAway() and the wait
	 * for the mutex would call that as well */
	VirtMem *vm = t->getProc()->getVM();
	if(vm->tryAquire()) {
		for(size_t i = 0; i < STACK_REG_COUNT; i++) {
			if(t->getStackRegion(i))
				t->getStackRegion(i)->reg->setTimestamp(timestamp);
		}
		for(auto reg = vm->regtree.begin(); reg != vm->regtree.end(); ++reg) {
			if(!(reg->reg->getFlags() & RF_STACK))
				reg->reg->setTimestamp(timestamp);
		}
		vm->release();
	}
}

size_t VirtMem::getMemUsage(size_t *pages) const {
	size_t rpages = 0;
	*pages = 0;
	acquire();
	for(auto vm = regtree.cbegin(); vm != regtree.cend(); ++vm) {
		size_t count = 0;
		size_t pageCount = BYTES_2_PAGES(vm->reg->getByteCount());
		for(size_t j = 0; j < pageCount; j++) {
			if(!(vm->reg->getPageFlags(j) & (PF_SWAPPED | PF_DEMANDLOAD | PF_COPYONWRITE)))
				count++;
		}
		*pages += pageCount;
		/* to prevent floating point arithmetic, multiply it with the page-size */
		rpages += (count * PAGE_SIZE) / vm->reg->refCount();
	}
	release();
	return rpages;
}

void VirtMem::getRegRange(VMRegion *vm,uintptr_t *start,uintptr_t *end,bool locked) const {
	if(locked)
		acquire();
	if(start)
		*start = vm->virt();
	if(end)
		*end = vm->virt() + vm->reg->getByteCount();
	if(locked)
		release();
}

int VirtMem::getRegRange(uintptr_t virt,uintptr_t *start,uintptr_t *end) {
	int res = 0;
	acquire();
	VMRegion *reg = regtree.getByAddr(virt);
	if(reg)
		getRegRange(reg,start,end,false);
	else
		res = -ENXIO;
	release();
	return res;
}

ssize_t VirtMem::getShareInfo(uintptr_t addr,char *path,size_t size) {
	ssize_t res = -ENXIO;
	acquire();
	VMRegion *reg = regtree.getByAddr(addr);
	if(!reg)
		goto error;
	if(reg->virt() != addr || ~reg->reg->getFlags() & RF_SHAREABLE || !reg->reg->getFile()) {
		res = -EINVAL;
		goto error;
	}
	reg->reg->getFile()->getPathTo(path,size);
	res = reg->reg->getByteCount();
error:
	release();
	return res;
}

int VirtMem::pagefault(uintptr_t addr,bool write) {
	Thread *t = Thread::getRunning();
	VMRegion *vmreg;

	/* we can swap here; note that we don't need page-tables in this case, they're always present */
	if(!t->reserveFrames(1))
		return -ENOMEM;

	VirtMem *vm = t->getProc()->getVM();
	vm->acquire();
	vmreg = vm->regtree.getByAddr(addr);
	if(vmreg == NULL) {
		vm->release();
		t->discardFrames();
		return -EFAULT;
	}
	vmreg->reg->acquire();
	int res = vm->doPagefault(addr,vmreg,write);
	vmreg->reg->release();
	vm->release();
	t->discardFrames();
	return res;
}

int VirtMem::doPagefault(uintptr_t addr,VMRegion *vm,bool write) {
	int res;
	size_t page = (addr - vm->virt()) / PAGE_SIZE;
	ulong flags = vm->reg->getPageFlags(page);
	addr &= ~(PAGE_SIZE - 1);
	if(flags & PF_DEMANDLOAD) {
		res = demandLoad(vm,addr);
		if(res == 0)
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
		res = 0;
	}
	else {
		vassert(flags == 0,"Flags: %x",flags);
		/* its ok if its either a read-access or a write-access on a writeable region */
		/* note: this map happen if e.g. two threads of a process cause a page-fault simultanously.
		 * of course, only one thread gets the region-mutex and handles the page-fault. the other
		 * one will get the mutex afterwards and the flags will be zero, because the page-fault
		 * has already been handled. */
		res = (!write || (vm->reg->getFlags() & RF_WRITABLE)) ? 0 : -EFAULT;
	}
	return res;
}

void VirtMem::unmapAll(bool remStack) {
	acquire();
	for(auto vm = regtree.begin(); vm != regtree.end(); ) {
		auto old = vm++;
		if((!(old->reg->getFlags() & RF_STACK) || remStack))
			doUnmap(&*old);
	}
	release();
}

void VirtMem::unmap(VMRegion *vm) {
	acquire();
	doUnmap(vm);
	release();
}

int VirtMem::unmap(uintptr_t virt) {
	int res = 0;
	acquire();
	VMRegion *vm = regtree.getByAddr(virt);
	if(vm)
		doUnmap(vm);
	else
		res = -ENXIO;
	release();
	return res;
}

void VirtMem::sync(VMRegion *vm) const {
	OpenFile *file = vm->reg->getFile();
	if((vm->reg->getFlags() & RF_SHAREABLE) && (vm->reg->getFlags() & RF_WRITABLE) && file) {
		size_t pcount = BYTES_2_PAGES(vm->reg->getByteCount());
		pid_t pid = proc->getPid();
		/* can't and won't be done by a different process */
		assert(pid == Proc::getRunning());

		for(size_t i = 0; i < pcount; i++) {
			size_t amount = i < pcount - 1 ? PAGE_SIZE : (vm->reg->getByteCount() - i * PAGE_SIZE);
			/* if the page isn't present, skip it */
			if(vm->reg->getPageFlags(i) & (PF_DEMANDLOAD | PF_SWAPPED))
				continue;

			/* just ignore the write-back if we can't seek */
			if(file->seek(pid,vm->reg->getOffset() + i * PAGE_SIZE,SEEK_SET) < 0)
				return;

			/* write our mapped memory to file */
			file->write(pid,(void*)(vm->virt() + i * PAGE_SIZE),amount);
		}
	}
}

void VirtMem::doUnmap(VMRegion *vm) {
	vm->reg->acquire();
	size_t pcount = BYTES_2_PAGES(vm->reg->getByteCount());
	sassert(vm->reg->remFrom(this));
	PageTables::NoAllocator alloc;
	if(vm->reg->refCount() == 0) {
		uintptr_t virt = vm->virt();
		/* first, write the content of the memory back to the file, if necessary */
		sync(vm);
		/* remove us from cow and unmap the pages (and free frames, if necessary) */
		for(size_t i = 0; i < pcount; i++) {
			bool freeFrame = !(vm->reg->getFlags() & RF_NOFREE);
			frameno_t frameNo = 0;
			if(vm->reg->getPageFlags(i) & PF_COPYONWRITE) {
				bool foundOther;
				frameNo = getPageDir()->getFrameNo(virt);
				/* we can free the frame if there is no other user */
				addShared(-CopyOnWrite::remove(frameNo,&foundOther));
				freeFrame = !foundOther;
			}

			if(vm->reg->getPageFlags(i) & PF_SWAPPED)
				addSwap(-1);
			else if(!(vm->reg->getPageFlags(i) & (PF_COPYONWRITE | PF_DEMANDLOAD))) {
				if(freeFrame) {
					if(frameNo == 0)
						frameNo = getPageDir()->getFrameNo(virt);
					PhysMem::free(frameNo,PhysMem::USR);
				}

				if(vm->reg->getFlags() & (RF_NOFREE | RF_SHAREABLE))
					addShared(-1);
				else
					addOwn(-1);
			}

			virt += PAGE_SIZE;
		}

		/* now unmap it (do it here to prevent multiple calls for it (locking, ...) */
		getPageDir()->unmap(vm->virt(),pcount,alloc);
		addOwn(-alloc.pageTables());

		/* store next free stack-address, if its a stack */
		if(vm->virt() + vm->reg->getByteCount() > freeStackAddr && (vm->reg->getFlags() & RF_STACK))
			freeStackAddr = vm->virt() + vm->reg->getByteCount();
		/* give the memory back to the free-area, if its in there */
		else if(vm->virt() >= FREE_AREA_BEGIN)
			freemap.free(vm->virt(),ROUND_PAGE_UP(vm->reg->getByteCount()));
		if(vm->virt() == dataAddr)
			dataAddr = 0;
		/* remove from shared tree */
		if(vm->reg->getFlags() & RF_SHAREABLE)
			ShFiles::remove(vm);
		/* now destroy region */
		vm->reg->release();
		/* do the remove BEFORE the free */
		Region *r = vm->reg;
		regtree.remove(vm);
		delete r;
	}
	else {
		size_t sw;
		/* no free here, just unmap */
		getPageDir()->unmap(vm->virt(),pcount,alloc);
		/* in this case its always a shared region because otherwise there wouldn't be other users */
		/* so we have to substract the present content-frames from the shared ones,
		 * and the ptables from ours */
		addShared(-vm->reg->pageCount(&sw));
		addOwn(-alloc.pageTables());
		addSwap(-sw);
		/* remove from shared tree */
		if(vm->reg->getFlags() & RF_SHAREABLE)
			ShFiles::remove(vm);
		/* give the memory back to the free-area, if its in there */
		if(vm->virt() >= FREE_AREA_BEGIN)
			freemap.free(vm->virt(),ROUND_PAGE_UP(vm->reg->getByteCount()));
		vm->reg->release();
		regtree.remove(vm);
	}
}

int VirtMem::join(uintptr_t srcAddr,VirtMem *dst,VMRegion **nvm,uintptr_t *dstAddr,ulong flags) {
	size_t swCount,pageCount,presentCount;
	size_t oldSh,oldOwn;
	uintptr_t addr;
	int pts,res = -ENOMEM;
	PageTables::NoAllocator alloc;
	Thread *t = Thread::getRunning();
	if(dst == this)
		return -EEXIST;

	acquire();
	// better use tryAquire here to prevent a deadlock; this is of course suboptimal because the
	// caller would have to retry the operation in this case
	if(!dst->tryAquire()) {
		release();
		return -ESRCH;
	}

	VMRegion *vm = regtree.getByAddr(srcAddr);
	if(vm == NULL) {
		res = -ENXIO;
		goto errProc;
	}

	vm->reg->acquire();
	assert(vm->reg->getFlags() & RF_SHAREABLE);

	pageCount = BYTES_2_PAGES(vm->reg->getByteCount());
	presentCount = vm->reg->pageCount(&swCount);

	/* should we populate it but fail if there is not enough mem? */
	if((flags & (MAP_POPULATE | MAP_NOSWAP)) == (MAP_POPULATE | MAP_NOSWAP)) {
		if(!t->reserveFrames(pageCount - presentCount,false))
			goto errRel;
	}

	/* check whether the process has already mapped this region */
	/* since this is not really necessary, we forbid it for simplicity */
	if(dst->regtree.getByReg(vm->reg) != NULL) {
		res = -EEXIST;
		goto errRel;
	}

	addr = dstAddr ? *dstAddr : 0;
	if(addr == 0 || (~flags & MAP_FIXED))
		addr = dst->freemap.allocate(ROUND_PAGE_UP(vm->reg->getByteCount()));
	else if(dst->regtree.getByAddr(addr) != NULL)
		goto errRel;
	if(addr == 0)
		goto errRel;
	*nvm = dst->regtree.add(vm->reg,addr);
	if(*nvm == NULL)
		goto errReg;
	if(!vm->reg->addTo(dst))
		goto errAdd;

	/* shared, so content-frames to shared, ptables to own */
	pts = getPageDir()->clone(dst->getPageDir(),vm->virt(),(*nvm)->virt(),pageCount,true);
	if(pts < 0)
		goto errRem;

	oldSh = dst->getSharedFrames();
	oldOwn = dst->getOwnFrames();

	/* fault-in all pages that are not present yet */
	if(flags & MAP_POPULATE) {
		res = populatePages(*nvm,pageCount);
		if(res < 0)
			goto errUnmap;
	}

	if((res = ShFiles::add(*nvm,dst->getProc()->getPid())) < 0)
		goto errUnmap;

	dst->addShared(presentCount);
	dst->addOwn(pts);
	dst->addSwap(swCount);

	/* lock the region, if desired */
	if(flags & MAP_LOCKED)
		(*nvm)->reg->setFlags((*nvm)->reg->getFlags() | RF_LOCKED);

	if(dstAddr)
		*dstAddr = addr;
	vm->reg->release();
	dst->release();
	release();
	return 0;

errUnmap:
	dst->addShared(oldSh - dst->getSharedFrames());
	dst->addOwn(oldOwn - dst->getOwnFrames());
	getPageDir()->unmap(vm->virt(),pageCount,alloc);
errRem:
	vm->reg->remFrom(dst);
errAdd:
	dst->regtree.remove((*nvm));
errReg:
	if(dstAddr == 0)
		dst->freemap.free(addr,ROUND_PAGE_UP(vm->reg->getByteCount()));
errRel:
	t->discardFrames();
	vm->reg->release();
errProc:
	dst->release();
	release();
	return res;
}

int VirtMem::cloneAll(VirtMem *dst) {
	Thread *t = Thread::getRunning();
	VMTree::iterator vm;
	VMRegion *nvm;
	Region *reg;
	size_t j;
	uintptr_t virt;
	assert(proc == t->getProc());

	acquire();
	if(!dst->tryAquire()) {
		release();
		return -ESRCH;
	}

	dst->swapped = 0;
	dst->sharedFrames = 0;
	dst->ownFrames = 0;
	dst->dataAddr = dataAddr;

	VMTree::addTree(dst,&dst->regtree);
	for(vm = regtree.begin(); vm != regtree.end(); ++vm) {
		/* just clone the tls- and stack-region of the current thread */
		if(!(vm->reg->getFlags() & RF_STACK) || t->hasStackRegion(&*vm)) {
			vm->reg->acquire();
			/* TODO ?? better don't share the file; they may have to read in parallel */
			if(vm->reg->getFlags() & RF_SHAREABLE) {
				vm->reg->addTo(dst);
				reg = vm->reg;
			}
			else {
				reg = cloneObj<Region>(*vm->reg,dst);
				if(reg == NULL)
					goto errorRel;
			}

			nvm = dst->regtree.add(reg,vm->virt());
			if(nvm == NULL)
				goto errorReg;

			/* remove regions in the free area from the free-map */
			if(vm->virt() >= FREE_AREA_BEGIN) {
				if(!dst->freemap.allocateAt(nvm->virt(),ROUND_PAGE_UP(nvm->reg->getByteCount())))
					goto errorRem;
			}

			/* now copy the pages */
			size_t pageCount = BYTES_2_PAGES(nvm->reg->getByteCount());
			ssize_t res = getPageDir()->clone(dst->getPageDir(),vm->virt(),nvm->virt(),pageCount,
					vm->reg->getFlags() & RF_SHAREABLE);
			if(res < 0)
				goto errorFreeArea;
			dst->addOwn(res);

			/* update stats */
			if(vm->reg->getFlags() & RF_SHAREABLE) {
				size_t sw;
				dst->addShared(nvm->reg->pageCount(&sw));
				dst->addSwap(sw);
				/* ignore it here if it fails */
				ShFiles::add(nvm,dst->getProc()->getPid());
			}
			/* add frames to copy-on-write, if not shared */
			else {
				virt = nvm->virt();
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
	if(vm->virt() >= FREE_AREA_BEGIN)
		dst->freemap.free(nvm->virt(),ROUND_PAGE_UP(nvm->reg->getByteCount()));
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
	Proc::removeRegions(dst->proc->getPid(),true);
	/* no need to free regs, since vmm_remove has already done it */
	return -ENOMEM;
}

int VirtMem::growStackTo(VMRegion *vm,uintptr_t addr) {
	int res = -EFAULT;
	acquire();
	ssize_t newPages = 0;
	addr &= ~(PAGE_SIZE - 1);
	/* report failure if its outside (upper / lower) of the region */
	/* note that we assume here that if a thread has multiple stack-regions, they grow towards
	 * each other */
	if(vm->reg->getFlags() & RF_GROWS_DOWN) {
		if(addr < vm->virt() + ROUND_PAGE_UP(vm->reg->getByteCount()) && addr < vm->virt()) {
			res = 0;
			newPages = (vm->virt() - addr) / PAGE_SIZE;
		}
	}
	else {
		if(addr >= vm->virt() && addr >= vm->virt() + ROUND_PAGE_UP(vm->reg->getByteCount())) {
			res = 0;
			newPages = ROUND_PAGE_UP(addr -
					(vm->virt() + ROUND_PAGE_UP(vm->reg->getByteCount()) - 1)) / PAGE_SIZE;
		}
	}

	if(res == 0) {
		/* if its too much, try the next one; if there is none that fits, report failure */
		if(BYTES_2_PAGES(vm->reg->getByteCount()) + newPages >= MAX_STACK_PAGES - 1)
			res = -EFAULT;
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
	return res;
}

size_t VirtMem::grow(uintptr_t addr,ssize_t amount) {
	acquire();
	size_t res = 0;
	VMRegion *vm = regtree.getByAddr(addr);
	if(vm)
		res = doGrow(vm,amount);
	release();
	return res;
}

size_t VirtMem::doGrow(VMRegion *vm,ssize_t amount) {
	ssize_t res;
	vm->reg->acquire();
	assert((vm->reg->getFlags() & RF_GROWABLE) && !(vm->reg->getFlags() & RF_SHAREABLE));

	/* check whether we've reached the max stack-pages */
	if((vm->reg->getFlags() & RF_STACK) &&
			BYTES_2_PAGES(vm->reg->getByteCount()) + amount >= MAX_STACK_PAGES - 1) {
		vm->reg->release();
		return 0;
	}

	/* resize region */
	PageTables::UAllocator alloc;
	uintptr_t oldVirt = vm->virt();
	size_t oldSize = vm->reg->getByteCount();
	if(amount != 0) {
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
		size_t own = 0;
		if((res = vm->reg->grow(amount,&own)) < 0) {
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
				vm->virt(vm->virt() - amount * PAGE_SIZE);
				virt = vm->virt();
			}
			else
				virt = vm->virt() + ROUND_PAGE_UP(oldSize);
			res = getPageDir()->map(virt,amount,alloc,mapFlags);
			if(res < 0) {
				if(vm->reg->getFlags() & RF_GROWS_DOWN)
					vm->virt(vm->virt() + amount * PAGE_SIZE);
				vm->reg->release();
				return 0;
			}
			addOwn(alloc.pageTables() + amount);
			/* remove it from the free area (used by the dynlinker for its data-region only) */
			/* note that we assume here that we get the location at the end of the data-region */
			if(vm->virt() >= FREE_AREA_BEGIN)
				freemap.allocate(amount * PAGE_SIZE);
		}
		else {
			if(vm->reg->getFlags() & RF_GROWS_DOWN) {
				virt = vm->virt();
				vm->virt(vm->virt() - amount * PAGE_SIZE);
			}
			else
				virt = vm->virt() + ROUND_PAGE_UP(vm->reg->getByteCount());
			/* give it back to the free area */
			if(vm->virt() >= FREE_AREA_BEGIN)
				freemap.free(virt,-amount * PAGE_SIZE);
			getPageDir()->unmap(virt,-amount,alloc);
			addOwn(-(alloc.pageTables() - own));
			addSwap(-res);
		}
	}

	res = (vm->reg->getFlags() & RF_GROWS_DOWN) ? oldVirt : oldVirt + ROUND_PAGE_UP(oldSize);
	vm->reg->release();
	return res;
}

const char *VirtMem::getRegName(const VMRegion *vm) const {
	const char *name = "";
	if(vm->virt() == dataAddr)
		name = "data";
	else if(vm->reg->getFlags() & RF_STACK)
		name = "stack";
	else if(vm->reg->getFlags() & RF_NOFREE)
		name = "phys";
	else if(vm->reg->getFile())
		name = vm->reg->getFile()->getPath();
	else {
		if(proc->getEntryPoint() >= vm->virt() && proc->getEntryPoint() < vm->virt() + vm->reg->getByteCount())
			name = proc->getProgram();
		else
			name = "???";
	}
	return name;
}

void VirtMem::printMaps(OStream &os) const {
	acquire();
	for(auto vm = regtree.cbegin(); vm != regtree.cend(); ++vm) {
		vm->reg->acquire();
		os.writef("%-22s %p..%p (%5zuK) %c%c%c%c%c%c%c",getRegName(&*vm),vm->virt(),
				vm->virt() + vm->reg->getByteCount() - 1,vm->reg->getByteCount() / 1024,
				(vm->reg->getFlags() & RF_WRITABLE) ? 'W' : 'w',
				(vm->reg->getFlags() & RF_EXECUTABLE) ? 'X' : 'x',
				(vm->reg->getFlags() & RF_GROWABLE) ? 'G' : 'g',
				(vm->reg->getFlags() & RF_SHAREABLE) ? 'S' : 's',
				(vm->reg->getFlags() & RF_LOCKED) ? 'L' : 'l',
				(vm->reg->getFlags() & RF_GROWS_DOWN) ? 'D' : 'd',
				(vm->reg->getFlags() & RF_NOFREE) ? 'f' : 'F');
		vm->reg->release();
		os.writef("\n");
	}
	release();
}

void VirtMem::printShort(OStream &os,const char *prefix) const {
	acquire();
	for(auto vm = regtree.cbegin(); vm != regtree.cend(); ++vm) {
		os.writef("%s%-22s %p..%p (%5zuK) ",prefix,getRegName(&*vm),vm->virt(),
				vm->virt() + vm->reg->getByteCount() - 1,vm->reg->getByteCount() / 1024);
		vm->reg->printFlags(os);
		os.writef("\n");
	}
	release();
}

void VirtMem::printRegions(OStream &os) const {
	acquire();
	size_t c = 0;
	for(auto vm = regtree.cbegin(); vm != regtree.cend(); ++vm) {
		if(c)
			os.writef("\n");
		vm->reg->acquire();
		os.writef("VMRegion (%p .. %p):\n",vm->virt(),vm->virt() + vm->reg->getByteCount() - 1);
		vm->reg->print(os,vm->virt());
		vm->reg->release();
		c++;
	}
	if(c == 0)
		os.writef("- no regions -\n");
	release();
}

void VirtMem::print(OStream &os) const {
	freemap.print(os);
	os.writef("Regions:\n");
	os.writef("\tDataRegion: %p\n",dataAddr);
	os.writef("\tFreeStack: %p\n",freeStackAddr);
	printShort(os,"\t");
}

int VirtMem::demandLoad(VMRegion *vm,uintptr_t addr) {
	int res;

	/* calculate the number of bytes to load and zero */
	size_t loadCount = 0, zeroCount;
	if(addr - vm->virt() < vm->reg->getLoadCount())
		loadCount = MIN(PAGE_SIZE,vm->reg->getLoadCount() - (addr - vm->virt()));
	zeroCount = MIN(PAGE_SIZE,vm->reg->getByteCount() - (addr - vm->virt())) - loadCount;

	/* load from file */
	if(loadCount)
		res = loadFromFile(vm,addr,loadCount);
	else
		res = 0;

	/* zero the rest, if necessary */
	if(res == 0 && zeroCount) {
		/* do the memclear before the mapping to ensure that it's ready when the first CPU sees it */
		frameno_t frame = loadCount ? Proc::getCurPageDir()->getFrameNo(addr)
									: Thread::getRunning()->getFrame();
		uintptr_t frameAddr = PageDir::getAccess(frame);
		memclear((void*)(frameAddr + loadCount),zeroCount);
		PageDir::removeAccess(frame);
		/* if the pages weren't present so far, map them into every process that has this region */
		if(!loadCount) {
			uint mapFlags = PG_PRESENT;
			if(vm->reg->getFlags() & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			/* this doesn't seem to make a lot of sense but is necessary for initloader */
			if(vm->reg->getFlags() & RF_EXECUTABLE)
				mapFlags |= PG_EXECUTABLE;
			for(auto mp = vm->reg->vmbegin(); mp != vm->reg->vmend(); ++mp) {
				/* the region may be mapped to a different virtual address */
				PageTables::RangeAllocator alloc(frame);
				VMRegion *mpreg = (*mp)->regtree.getByReg(vm->reg);
				/* can't fail */
				sassert((*mp)->getPageDir()->map(mpreg->virt() + (addr - vm->virt()),1,alloc,mapFlags) == 0);
				if(vm->reg->getFlags() & RF_SHAREABLE)
					(*mp)->addShared(1);
				else
					(*mp)->addOwn(1);
			}
		}
	}
	return res;
}

int VirtMem::loadFromFile(VMRegion *vm,uintptr_t addr,size_t loadCount) {
	uint mapFlags;
	frameno_t frame;
	void *tempBuf;
	/* note that we currently ignore that the file might have changed in the meantime */
	ssize_t err;
	off_t pos = vm->reg->getOffset() + (addr - vm->virt());
	if((err = vm->reg->getFile()->seek(proc->getPid(),pos,SEEK_SET)) < 0)
		goto error;

	/* first read into a temp-buffer because we can't mark the page as present until
	 * its read from disk. and we can't use a temporary mapping when switching
	 * threads. */
	tempBuf = Cache::alloc(PAGE_SIZE);
	if(tempBuf == NULL) {
		err = -ENOMEM;
		goto error;
	}
	err = vm->reg->getFile()->read(proc->getPid(),tempBuf,loadCount);
	if(err != (ssize_t)loadCount) {
		if(err >= 0)
			err = -ENOMEM;
		goto errorFree;
	}

	/* copy into frame */
	frame = PageDir::demandLoad(tempBuf,loadCount,vm->reg->getFlags());

	/* free resources not needed anymore */
	Cache::free(tempBuf);

	/* map into all pagedirs */
	{
		mapFlags = PG_PRESENT;
		if(vm->reg->getFlags() & RF_WRITABLE)
			mapFlags |= PG_WRITABLE;
		if(vm->reg->getFlags() & RF_EXECUTABLE)
			mapFlags |= PG_EXECUTABLE;
		for(auto mp = vm->reg->vmbegin(); mp != vm->reg->vmend(); ++mp) {
			PageTables::RangeAllocator alloc(frame);
			/* the region may be mapped to a different virtual address */
			VMRegion *mpreg = (*mp)->regtree.getByReg(vm->reg);
			/* can't fail */
			sassert((*mp)->getPageDir()->map(mpreg->virt() + (addr - vm->virt()),1,alloc,mapFlags) == 0);
			if(vm->reg->getFlags() & RF_SHAREABLE)
				(*mp)->addShared(1);
			else
				(*mp)->addOwn(1);
		}
	}
	return 0;

errorFree:
	Cache::free(tempBuf);
error:
	Log::get().writef("Demandload page @ %p for proc %s: %s (%d)\n",addr,
		proc->getProgram(),strerror(err),err);
	return err;
}

Region *VirtMem::getLRURegion() {
	Region *lru = NULL;
	uint64_t ts = (uint64_t)-1;
	VMTree *tree = VMTree::reqTree();
	for(; tree != NULL; tree = tree->getNext()) {
		/* same as below; we have to try to acquire the mutex, otherwise we risk a deadlock */
		if(tree->getVM()->tryAquire()) {
			for(auto vm = tree->cbegin(); vm != tree->cend(); ++vm) {
				size_t count = 0;
				if(vm->reg->getTimestamp() >= ts)
					continue;

				/* we can't block here because otherwise we risk a deadlock. suppose that fs has to
				 * swap out to get more memory. if we want to demand-load something before this operation
				 * is finished and lock the region for that, the swapper will find this region at this
				 * place locked. so we have to skip it in this case to be able to continue. */
				if(vm->reg->tryAquire()) {
					/* skip locked regions */
					if(~vm->reg->getFlags() & RF_LOCKED) {
						size_t pages = BYTES_2_PAGES(vm->reg->getByteCount());
						for(size_t j = 0; j < pages; j++) {
							if(!(vm->reg->getPageFlags(j) & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD))) {
								count++;
								break;
							}
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
	PageTables::NoAllocator alloc;
	reg->setPageFlags(index,reg->getPageFlags(index) | PF_SWAPPED);
	for(auto mp = reg->vmbegin(); mp != reg->vmend(); ++mp) {
		/* the region may be mapped to a different virtual address */
		VMRegion *mpreg = (*mp)->regtree.getByReg(reg);
		/* can't fail */
		sassert((*mp)->getPageDir()->map(mpreg->virt() + offset,1,alloc,0) == 0);
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
		PageTables::RangeAllocator alloc(frameNo);
		/* the region may be mapped to a different virtual address */
		VMRegion *mpreg = (*mp)->regtree.getByReg(reg);
		/* can't fail */
		sassert((*mp)->getPageDir()->map(mpreg->virt() + offset,1,alloc,flags) == 0);
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
		end = dataReg->virt() + ROUND_PAGE_UP(dataReg->reg->getByteCount());
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
	size_t size = ROUND_PAGE_UP(byteCount);
	uintptr_t addr = freeStackAddr != 0 ? freeStackAddr : STACK_AREA_BEGIN;
	for(; addr < STACK_AREA_END; addr += MAX_STACK_PAGES * PAGE_SIZE) {
		if(isOccupied(addr,addr + (MAX_STACK_PAGES - 1) * PAGE_SIZE) == NULL) {
			freeStackAddr = addr + MAX_STACK_PAGES * PAGE_SIZE;
			if(rflags & RF_GROWS_DOWN)
				return addr + MAX_STACK_PAGES * PAGE_SIZE - size;
			return addr;
		}
	}
#endif
	return 0;
}

bool VirtMem::isOccupied(uintptr_t start,uintptr_t end) const {
	for(auto vm = regtree.cbegin(); vm != regtree.cend(); ++vm) {
		uintptr_t rstart = vm->virt();
		uintptr_t rend = vm->virt() + ROUND_PAGE_UP(vm->reg->getByteCount());
		if(OVERLAPS(rstart,rend,start,end))
			return true;
	}
	return false;
}

uintptr_t VirtMem::getFirstUsableAddr() const {
	uintptr_t addr = 0;
	for(auto vm = regtree.cbegin(); vm != regtree.cend(); ++vm) {
		if(!(vm->reg->getFlags() & RF_STACK) && vm->virt() < FREE_AREA_BEGIN &&
				vm->virt() + vm->reg->getByteCount() > addr) {
			addr = vm->virt() + vm->reg->getByteCount();
		}
	}
	return ROUND_PAGE_UP(addr);
}
