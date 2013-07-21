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
#include <sys/mem/vmm.h>
#include <sys/mem/cow.h>
#include <sys/mem/cache.h>
#include <sys/mem/swapmap.h>
#include <sys/task/event.h>
#include <sys/task/smp.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/log.h>
#include <sys/mutex.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

/**
 * The vmm-module manages the user-part of a process's virtual addressspace. That means it
 * manages the regions that are used by the process (as an instance of sVMRegion) and decides
 * where the regions are placed. So it binds a region to a virtual address via sVMRegion.
 */

#define DEBUG_SWAP			0

static sRegion *vmm_getLRURegion(void);
static ssize_t vmm_getPgIdxForSwap(const sRegion *reg);
static void vmm_setSwappedOut(sRegion *reg,size_t index);
static void vmm_setSwappedIn(sRegion *reg,size_t index,frameno_t frameNo);
static uintptr_t vmm_getBinary(sFile *file,pid_t *binOwner);
static bool vmm_doPagefault(uintptr_t addr,Proc *p,sVMRegion *vm,bool write);
static void vmm_doRemove(Proc *p,sVMRegion *vm);
static size_t vmm_doGrow(Proc *p,sVMRegion *vm,ssize_t amount);
static bool vmm_demandLoad(Proc *p,sVMRegion *vm,uintptr_t addr);
static bool vmm_loadFromFile(Proc *p,sVMRegion *vm,uintptr_t addr,size_t loadCount);
static uintptr_t vmm_findFreeStack(Proc *p,size_t byteCount,ulong rflags);
static sVMRegion *vmm_isOccupied(Proc *p,uintptr_t start,uintptr_t end);
static uintptr_t vmm_getFirstUsableAddr(Proc *p);
static Proc *vmm_reqProc(pid_t pid);
static Proc *vmm_tryReqProc(pid_t pid);
static void vmm_relProc(Proc *p);
static const char *vmm_getRegName(Proc *p,const sVMRegion *vm);

static uint8_t buffer[PAGE_SIZE];

void vmm_init(void) {
	/* nothing to do */
}

uintptr_t vmm_addPhys(pid_t pid,uintptr_t *phys,size_t bCount,size_t align,bool writable) {
	sVMRegion *vm;
	ssize_t res;
	Proc *p;
	size_t i,pages = BYTES_2_PAGES(bCount);
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
			for(i = 0; i < pages; i++)
				frames[i] = first + i;
		}
		else {
			for(i = 0; i < pages; i++)
				frames[i] = PhysMem::allocate(PhysMem::USR);
		}
	}
	/* otherwise use the specified one */
	else {
		for(i = 0; i < pages; i++) {
			frames[i] = *phys / PAGE_SIZE;
			*phys += PAGE_SIZE;
		}
	}

	/* create region */
	res = vmm_map(pid,0,bCount,0,PROT_READ | (writable ? PROT_WRITE : 0),
			*phys ? (MAP_NOMAP | MAP_NOFREE) : MAP_NOMAP,NULL,0,&vm);
	if(res < 0) {
		if(!*phys)
			PhysMem::freeContiguous(frames[0],pages);
		goto error;
	}

	/* map memory */
	p = vmm_reqProc(pid);
	if(!p)
		goto errorRem;
	res = paging_mapTo(p->getPageDir(),vm->virt,frames,pages,
			writable ? PG_PRESENT | PG_WRITABLE : PG_PRESENT);
	if(res < 0)
		goto errorRel;
	if(*phys) {
		/* the page-tables are ours, the pages may be used by others, too */
		p->addOwn(res);
		p->addShared(pages);
	}
	else {
		/* its our own mem; store physical address for the caller */
		p->addOwn(res);
		*phys = frames[0] * PAGE_SIZE;
	}
	vmm_relProc(p);
	Thread::remHeapAlloc(frames);
	Cache::free(frames);
	return vm->virt;

errorRel:
	vmm_relProc(p);
errorRem:
	vmm_remove(pid,vm);
error:
	Thread::remHeapAlloc(frames);
	Cache::free(frames);
	return 0;
}

int vmm_map(pid_t pid,uintptr_t addr,size_t length,size_t loadCount,int prot,int flags,sFile *f,
            off_t offset,sVMRegion **vmreg) {
	sRegion *reg;
	sVMRegion *vm;
	int res;
	ulong pgFlags = f != NULL ? PF_DEMANDLOAD : 0;
	ulong rflags = flags | prot;
	Proc *p;
	assert(length > 0 && length >= loadCount);

	/* for files: try to find another process with that file */
	if(f && (rflags & MAP_SHARED)) {
		pid_t binOwner = 0;
		uintptr_t binVirt = vmm_getBinary(f,&binOwner);
		if(binVirt != 0)
			return vmm_join(binOwner,binVirt,pid,vmreg,rflags & MAP_FIXED ? addr : 0);
	}

	/* get the attributes of the region (depending on type) */
	p = vmm_reqProc(pid);
	if(!p)
		return -ESRCH;

	/* if it's a data-region.. */
	if((rflags & (PROT_WRITE | MAP_GROWABLE | MAP_STACK)) == (PROT_WRITE | MAP_GROWABLE)) {
		/* if we already have the real data-region */
		if(p->dataAddr && p->dataAddr < FREE_AREA_BEGIN) {
			/* using a fixed address is not allowed in this case */
			if(rflags & MAP_FIXED)
				return -EINVAL;
			/* in every case, it can't be growable */
			rflags &= ~MAP_GROWABLE;
		}
		/* otherwise, it's either the dynlink-data-region or the real one. both is ok */
	}

	/* determine address */
	res = -EINVAL;
	if(rflags & MAP_FIXED) {
		/* ok, the user has choosen a place. check it */
		if(!vmreg_available(&p->regtree,addr,length))
			goto errProc;
	}
	else {
		/* find a suitable place */
		if(rflags & MAP_STACK)
			addr = vmm_findFreeStack(p,length,rflags);
		else
			addr = vmfree_allocate(&p->freemap,ROUND_PAGE_UP(length));
		if(addr == 0)
			goto errProc;
	}

	/* create region */
	res = -ENOMEM;
	reg = reg_create(f,length,loadCount,offset,pgFlags,rflags);
	if(!reg)
		goto errProc;
	if(!reg_addTo(reg,p))
		goto errReg;
	vm = vmreg_add(&p->regtree,reg,addr);
	if(vm == NULL)
		goto errAdd;

	/* map into process */
	if(~rflags & MAP_NOMAP) {
		ssize_t pts;
		uint mapFlags = 0;
		size_t pageCount = BYTES_2_PAGES(vm->reg->byteCount);
		if(!(pgFlags & PF_DEMANDLOAD)) {
			mapFlags |= PG_PRESENT;
			if(rflags & PROT_EXEC)
				mapFlags |= PG_EXECUTABLE;
		}
		if(rflags & PROT_WRITE)
			mapFlags |= PG_WRITABLE;
		pts = paging_mapTo(p->getPageDir(),addr,NULL,pageCount,mapFlags);
		if(pts < 0)
			goto errMap;
		if(rflags & MAP_SHARED)
			p->addShared(pageCount);
		else
			p->addOwn(pageCount);
		p->addOwn(pts);
	}
	/* set data-region */
	if((rflags & (PROT_WRITE | MAP_GROWABLE | MAP_STACK)) == (PROT_WRITE | MAP_GROWABLE))
		p->dataAddr = addr;
	vmm_relProc(p);

	if(DISABLE_DEMLOAD || (rflags & MAP_POPULATE)) {
		size_t i;
		/* ensure that we don't discard frames that we might need afterwards */
		Thread *t = Thread::getRunning();
		size_t oldres = t->getReservedFrmCnt();
		t->discardFrames();
		for(i = 0; i < vm->reg->pfSize; i++)
			vmm_pagefault(vm->virt + i * PAGE_SIZE,false);
		t->reserveFrames(oldres);
	}
	*vmreg = vm;
	return 0;

errMap:
	vmreg_remove(&p->regtree,vm);
errAdd:
	reg_remFrom(reg,p);
errReg:
	reg_destroy(reg);
errProc:
	vmm_relProc(p);
	return res;
}

int vmm_regctrl(pid_t pid,uintptr_t addr,ulong flags) {
	size_t i,pgcount;
	sSLNode *n;
	sVMRegion *vmreg;
	Proc *p;
	int res = -EPERM;

	p = vmm_reqProc(pid);
	if(!p)
		return -ESRCH;

	vmreg = vmreg_getByAddr(&p->regtree,addr);
	if(vmreg == NULL) {
		vmm_relProc(p);
		return -ENXIO;
	}

	mutex_aquire(&vmreg->reg->lock);
	if(!vmreg || (vmreg->reg->flags & (RF_NOFREE | RF_STACK | RF_TLS)))
		goto error;

	/* check if COW is enabled for a page */
	pgcount = BYTES_2_PAGES(vmreg->reg->byteCount);
	for(i = 0; i < pgcount; i++) {
		if(vmreg->reg->pageFlags[i] & PF_COPYONWRITE)
			goto error;
	}

	/* change reg flags */
	vmreg->reg->flags &= ~(RF_WRITABLE | RF_EXECUTABLE);
	vmreg->reg->flags |= flags;

	/* change mapping */
	for(n = sll_begin(&vmreg->reg->procs); n != NULL; n = n->next) {
		Proc *mp = (Proc*)n->data;
		/* the region may be mapped to a different virtual address */
		sVMRegion *mpreg = vmreg_getByReg(&mp->regtree,vmreg->reg);
		assert(mpreg != NULL);
		for(i = 0; i < pgcount; i++) {
			/* determine flags; we can't always mark it present.. */
			uint mapFlags = PG_KEEPFRM;
			if(!(vmreg->reg->pageFlags[i] & (PF_DEMANDLOAD | PF_SWAPPED)))
				mapFlags |= PG_PRESENT;
			if(flags & RF_EXECUTABLE)
				mapFlags |= PG_EXECUTABLE;
			if(flags & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			/* can't fail because of PG_KEEPFRM and because the page-table is always present */
			assert(paging_mapTo(mp->getPageDir(),mpreg->virt + i * PAGE_SIZE,NULL,1,mapFlags) == 0);
		}
	}
	res = 0;

error:
	mutex_release(&vmreg->reg->lock);
	vmm_relProc(p);
	return res;
}

void vmm_swapOut(pid_t pid,sFile *file,size_t count) {
	while(count > 0) {
		sRegion *reg = vmm_getLRURegion();
		if(reg == NULL)
			util_panic("No pages to swap out");

		/* the list of processes may have changed in the meantime; we can't swap out in this case */
		while(count > 0) {
			frameno_t frameNo;
			ulong block;
			ssize_t index;
			Proc *proc;
			sVMRegion *vmreg;

			/* get page to swap out */
			index = vmm_getPgIdxForSwap(reg);
			if(index < 0)
				break;

			/* get VM-region of first process */
			proc = (Proc*)sll_get(&reg->procs,0);
			vmreg = vmreg_getByReg(&proc->regtree,reg);

			/* find swap-block */
			block = swmap_alloc();
			assert(block != INVALID_BLOCK);

#if DEBUG_SWAP
			sSLNode *n;
			log_printf("OUT: %d of region %x (block %d)\n",index,vmreg->reg,block);
			for(n = sll_begin(reg->procs); n != NULL; n = n->next) {
				Proc *mp = (Proc*)n->data;
				sVMRegion *mpreg = vmreg_getByReg(t,&mp->regtree,reg);
				log_printf("\tProcess %d:%s -> page %p\n",mp->getPid(),mp->getCommand(),
						mpreg->virt + index * PAGE_SIZE);
			}
			log_printf("\n");
#endif

			/* get the frame first, because the page has to be present */
			frameNo = paging_getFrameNo(proc->getPageDir(),vmreg->virt + index * PAGE_SIZE);
			/* unmap the page in all processes and ensure that all CPUs have flushed their TLB */
			/* this way we know that nobody can still access the page; if someone tries, he will
			 * cause a page-fault and will wait until we release the region-mutex */
			vmm_setSwappedOut(reg,index);
			reg_setSwapBlock(reg,index,block);
			SMP::ensureTLBFlushed();

			/* copy to a temporary buffer because we can't use the temp-area when switching threads */
			paging_copyFromFrame(frameNo,buffer);
			PhysMem::free(frameNo,PhysMem::USR);

			/* write out on disk */
			assert(vfs_seek(pid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
			assert(vfs_writeFile(pid,file,buffer,PAGE_SIZE) == PAGE_SIZE);

			count--;
		}
		mutex_release(&reg->lock);
	}
}

bool vmm_swapIn(pid_t pid,sFile *file,Thread *t,uintptr_t addr) {
	frameno_t frame;
	ulong block;
	size_t index;
	sVMRegion *vmreg = vmreg_getByAddr(&t->getProc()->regtree,addr);
	if(!vmreg)
		return false;

	addr &= ~(PAGE_SIZE - 1);
	index = (addr - vmreg->virt) / PAGE_SIZE;

	/* not swapped anymore? so probably another process has already swapped it in */
	if(!(vmreg->reg->pageFlags[index] & PF_SWAPPED))
		return false;

	block = reg_getSwapBlock(vmreg->reg,index);

#if DEBUG_SWAP
	sSLNode *n;
	log_printf("IN: %d of region %x (block %d)\n",index,vmreg->reg,block);
	for(n = sll_begin(vmreg->reg->getProc()s); n != NULL; n = n->next) {
		Proc *mp = (Proc*)n->data;
		sVMRegion *mpreg = vmreg_getByReg(t,&mp->regtree,vmreg->reg);
		log_printf("\tProcess %d:%s -> page %p\n",mp->getPid(),mp->getCommand(),
				mpreg->virt + index * PAGE_SIZE);
	}
	log_printf("\n");
#endif

	/* read into buffer (note that we can use the same for swap-in and swap-out because its both
	 * done by the swapper-thread) */
	assert(vfs_seek(pid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
	assert(vfs_readFile(pid,file,buffer,PAGE_SIZE) == PAGE_SIZE);

	/* copy into a new frame */
	frame = t->getFrame();
	paging_copyToFrame(frame,buffer);

	/* mark as not-swapped and map into all affected processes */
	vmm_setSwappedIn(vmreg->reg,index,frame);
	/* free swap-block */
	swmap_free(block);
	return true;
}

void vmm_setTimestamp(Thread *t,uint64_t timestamp) {
	if(!PhysMem::shouldSetRegTimestamp())
		return;
	/* ignore setting the timestamp if the process is currently locked; its not that critical and
	 * we can't wait for the mutex here because this is called from Thread::switchAway() and the wait
	 * for the mutex would call that as well */
	Proc *p = vmm_tryReqProc(t->getProc()->getPid());
	if(p) {
		sVMRegion *vm;
		size_t i;
		if(t->getTLSRegion())
			t->getTLSRegion()->reg->timestamp = timestamp;
		for(i = 0; i < STACK_REG_COUNT; i++) {
			if(t->getStackRegion(i))
				t->getStackRegion(i)->reg->timestamp = timestamp;
		}
		for(vm = t->getProc()->regtree.begin; vm != NULL; vm = vm->next) {
			if(!(vm->reg->flags & (RF_TLS | RF_STACK)))
				vm->reg->timestamp = timestamp;
		}
		vmm_relProc(p);
	}
}

void vmm_getMemUsageOf(Proc *p,size_t *own,size_t *shared,size_t *swapped) {
	*own = p->ownFrames;
	*shared = p->sharedFrames;
	*swapped = p->swapped;
}

float vmm_getMemUsage(pid_t pid,size_t *pages) {
	float rpages = 0;
	Proc *p = vmm_reqProc(pid);
	*pages = 0;
	if(p) {
		sVMRegion *vm;
		for(vm = p->regtree.begin; vm != NULL; vm = vm->next) {
			size_t j,count = 0;
			size_t pageCount = BYTES_2_PAGES(vm->reg->byteCount);
			for(j = 0; j < pageCount; j++) {
				if(!(vm->reg->pageFlags[j] & (PF_SWAPPED | PF_DEMANDLOAD | PF_COPYONWRITE)))
					count++;
			}
			*pages += pageCount;
			rpages += (float)count / reg_refCount(vm->reg);
		}
		vmm_relProc(p);
	}
	return rpages;
}

sVMRegion *vmm_getRegion(Proc *p,uintptr_t addr) {
	return vmreg_getByAddr(&p->regtree,addr);
}

bool vmm_getRegRange(pid_t pid,sVMRegion *vm,uintptr_t *start,uintptr_t *end,bool locked) {
	bool res = false;
	Proc *p = locked ? vmm_reqProc(pid) : Proc::getByPid(pid);
	if(p) {
		if(start)
			*start = vm->virt;
		if(end)
			*end = vm->virt + vm->reg->byteCount;
		if(locked)
			vmm_relProc(p);
		res = true;
	}
	return res;
}

static uintptr_t vmm_getBinary(sFile *file,pid_t *binOwner) {
	uintptr_t res = 0;
	sVMRegTree *tree = vmreg_reqTree();
	for(; tree != NULL; tree = tree->next) {
		sVMRegion *vm;
		for(vm = tree->begin; vm != NULL; vm = vm->next) {
			/* if its shareable and the binary fits, return region-number */
			if((vm->reg->flags & RF_SHAREABLE) && vm->reg->file && vfs_isSameFile(vm->reg->file,file)) {
				res = vm->virt;
				*binOwner = tree->pid;
				break;
			}
		}
	}
	vmreg_relTree();
	return res;
}

bool vmm_pagefault(uintptr_t addr,bool write) {
	Proc *p;
	Thread *t = Thread::getRunning();
	sVMRegion *vm;
	bool res = false;

	/* we can swap here; note that we don't need page-tables in this case, they're always present */
	if(!t->reserveFrames(1))
		return false;

	p = vmm_reqProc(t->getProc()->getPid());
	vm = vmreg_getByAddr(&p->regtree,addr);
	if(vm == NULL) {
		vmm_relProc(p);
		t->discardFrames();
		return false;
	}
	res = vmm_doPagefault(addr,p,vm,write);
	vmm_relProc(p);
	t->discardFrames();
	return res;
}

static bool vmm_doPagefault(uintptr_t addr,Proc *p,sVMRegion *vm,bool write) {
	bool res = false;
	ulong *flags;
	mutex_aquire(&vm->reg->lock);
	flags = vm->reg->pageFlags + (addr - vm->virt) / PAGE_SIZE;
	addr &= ~(PAGE_SIZE - 1);
	if(*flags & PF_DEMANDLOAD) {
		res = vmm_demandLoad(p,vm,addr);
		if(res)
			*flags &= ~PF_DEMANDLOAD;
	}
	else if(*flags & PF_SWAPPED)
		res = PhysMem::swapIn(addr);
	else if(*flags & PF_COPYONWRITE) {
		frameno_t frameNumber = paging_getFrameNo(p->getPageDir(),addr);
		size_t frmCount = CopyOnWrite::pagefault(addr,frameNumber);
		p->addOwn(frmCount);
		p->addShared(-frmCount);
		*flags &= ~PF_COPYONWRITE;
		res = true;
	}
	else {
		vassert(*flags == 0,"Flags: %x",*flags);
		/* its ok if its either a read-access or a write-access on a writeable region */
		/* note: this map happen if e.g. two threads of a process cause a page-fault simultanously.
		 * of course, only one thread gets the region-mutex and handles the page-fault. the other
		 * one will get the mutex afterwards and the flags will be zero, because the page-fault
		 * has already been handled. */
		res = !write || (vm->reg->flags & RF_WRITABLE);
	}
	mutex_release(&vm->reg->lock);
	return res;
}

void vmm_removeAll(pid_t pid,bool remStack) {
	Proc *p = vmm_reqProc(pid);
	if(p) {
		sVMRegion *vm;
		for(vm = p->regtree.begin; vm != NULL; vm = vm->next) {
			if((!(vm->reg->flags & RF_STACK) || remStack))
				vmm_doRemove(p,vm);
		}
		vmm_relProc(p);
	}
}

void vmm_remove(pid_t pid,sVMRegion *vm) {
	Proc *p = vmm_reqProc(pid);
	if(p) {
		vmm_doRemove(p,vm);
		vmm_relProc(p);
	}
}

static void vmm_sync(Proc *p,sVMRegion *vm) {
	if((vm->reg->flags & RF_SHAREABLE) && (vm->reg->flags & RF_WRITABLE) && vm->reg->file) {
		size_t i,pcount = BYTES_2_PAGES(vm->reg->byteCount);
		/* this is more complicated because p might not be the current process. in this case we
		 * can't directly access the memory. */
		pid_t cur = Proc::getRunning();
		uint8_t *buf;
		if(cur != p->getPid()) {
			buf = (uint8_t*)Cache::alloc(PAGE_SIZE);
			if(buf == NULL)
				return;
			Thread::addHeapAlloc(buf);
		}

		for(i = 0; i < pcount; i++) {
			size_t amount = i < pcount - 1 ? PAGE_SIZE : (vm->reg->byteCount - i * PAGE_SIZE);
			if(vfs_seek(p->getPid(),vm->reg->file,vm->reg->offset + i * PAGE_SIZE,SEEK_SET) < 0)
				return;
			if(p->getPid() == cur)
				vfs_writeFile(p->getPid(),vm->reg->file,(void*)(vm->virt + i * PAGE_SIZE),amount);
			else {
				/* we can't use the temp mapping during vfs_writeFile because we might perform a
				 * context-switch in between. */
				frameno_t frame = paging_getFrameNo(p->getPageDir(),vm->virt + i * PAGE_SIZE);
				uintptr_t addr = paging_getAccess(frame);
				memcpy(buf,(void*)addr,amount);
				paging_removeAccess();
				vfs_writeFile(p->getPid(),vm->reg->file,buf,amount);
			}
		}

		if(cur != p->getPid()) {
			Thread::remHeapAlloc(buf);
			Cache::free(buf);
		}
	}
}

static void vmm_doRemove(Proc *p,sVMRegion *vm) {
	size_t i = 0;
	size_t pcount;
	mutex_aquire(&vm->reg->lock);
	pcount = BYTES_2_PAGES(vm->reg->byteCount);
	assert(reg_remFrom(vm->reg,p));
	if(reg_refCount(vm->reg) == 0) {
		uintptr_t virt = vm->virt;
		/* first, write the content of the memory back to the file, if necessary */
		vmm_sync(p,vm);
		/* remove us from cow and unmap the pages (and free frames, if necessary) */
		for(i = 0; i < pcount; i++) {
			ssize_t pts;
			bool freeFrame = !(vm->reg->flags & RF_NOFREE);
			if(vm->reg->pageFlags[i] & PF_COPYONWRITE) {
				bool foundOther;
				frameno_t frameNo = paging_getFrameNo(p->getPageDir(),virt);
				/* we can free the frame if there is no other user */
				p->addShared(-CopyOnWrite::remove(frameNo,&foundOther));
				freeFrame = !foundOther;
				/* if we'll free the frame with unmap we will substract 1 too much because
				 * we don't own the frame */
				if(freeFrame)
					p->addOwn(1);
			}
			if(vm->reg->pageFlags[i] & PF_SWAPPED)
				p->addSwap(-1);
			pts = paging_unmapFrom(p->getPageDir(),virt,1,freeFrame);
			if(freeFrame) {
				if(vm->reg->flags & RF_SHAREABLE)
					p->addShared(-1);
				else
					p->addOwn(-1);
			}
			p->addOwn(-pts);
			virt += PAGE_SIZE;
		}
		/* store next free stack-address, if its a stack */
		if(vm->virt + vm->reg->byteCount > p->freeStackAddr && (vm->reg->flags & RF_STACK))
			p->freeStackAddr = vm->virt + vm->reg->byteCount;
		/* give the memory back to the free-area, if its in there */
		else if(vm->virt >= FREE_AREA_BEGIN)
			vmfree_free(&p->freemap,vm->virt,ROUND_PAGE_UP(vm->reg->byteCount));
		if(vm->virt == p->dataAddr)
			p->dataAddr = 0;
		/* now destroy region */
		mutex_release(&vm->reg->lock);
		reg_destroy(vm->reg);
	}
	else {
		size_t swapped;
		/* no free here, just unmap */
		ssize_t pts = paging_unmapFrom(p->getPageDir(),vm->virt,pcount,false);
		/* in this case its always a shared region because otherwise there wouldn't be other users */
		/* so we have to substract the present content-frames from the shared ones,
		 * and the ptables from ours */
		p->addShared(-reg_pageCount(vm->reg,&swapped));
		p->addOwn(-pts);
		p->addSwap(-swapped);
		/* give the memory back to the free-area, if its in there */
		if(vm->virt >= FREE_AREA_BEGIN)
			vmfree_free(&p->freemap,vm->virt,ROUND_PAGE_UP(vm->reg->byteCount));
		mutex_release(&vm->reg->lock);
	}
	vmreg_remove(&p->regtree,vm);
}

int vmm_join(pid_t srcId,uintptr_t srcAddr,pid_t dstId,sVMRegion **nvm,uintptr_t dstAddr) {
	Proc *src,*dst;
	ssize_t res;
	size_t pageCount,swapped;
	sVMRegion *vm;
	if(srcId == dstId)
		return -EINVAL;

	src = vmm_reqProc(srcId);
	dst = vmm_reqProc(dstId);
	if(!src || !dst) {
		if(dst)
			vmm_relProc(dst);
		if(src)
			vmm_relProc(src);
		return -ESRCH;
	}

	vm = vmreg_getByAddr(&src->regtree,srcAddr);
	if(vm == NULL)
		goto errProc;

	mutex_aquire(&vm->reg->lock);
	assert(vm->reg->flags & RF_SHAREABLE);

	if(dstAddr == 0)
		dstAddr = vmfree_allocate(&dst->freemap,ROUND_PAGE_UP(vm->reg->byteCount));
	else if(vmreg_getByAddr(&dst->regtree,dstAddr) != NULL)
		goto errReg;
	if(dstAddr == 0)
		goto errReg;
	*nvm = vmreg_add(&dst->regtree,vm->reg,dstAddr);
	if(*nvm == NULL)
		goto errReg;
	if(!reg_addTo(vm->reg,dst))
		goto errAdd;

	/* shared, so content-frames to shared, ptables to own */
	pageCount = BYTES_2_PAGES(vm->reg->byteCount);
	res = paging_clonePages(src->getPageDir(),dst->getPageDir(),vm->virt,(*nvm)->virt,pageCount,true);
	if(res < 0)
		goto errRem;
	dst->addShared(reg_pageCount(vm->reg,&swapped));
	dst->addOwn(res);
	dst->addSwap(swapped);
	mutex_release(&vm->reg->lock);
	vmm_relProc(dst);
	vmm_relProc(src);
	return 0;

errRem:
	reg_remFrom(vm->reg,dst);
errAdd:
	vmreg_remove(&dst->regtree,(*nvm));
errReg:
	mutex_release(&vm->reg->lock);
errProc:
	vmm_relProc(dst);
	vmm_relProc(src);
	return -ENOMEM;
}

int vmm_cloneAll(pid_t dstId) {
	size_t j;
	Thread *t = Thread::getRunning();
	Proc *dst = vmm_reqProc(dstId);
	Proc *src = vmm_reqProc(t->getProc()->getPid());
	sVMRegion *vm;
	dst->swapped = 0;
	dst->sharedFrames = 0;
	dst->ownFrames = 0;
	dst->dataAddr = src->dataAddr;

	vmreg_addTree(dstId,&dst->regtree);
	for(vm = src->regtree.begin; vm != NULL; vm = vm->next) {
		/* just clone the tls- and stack-region of the current thread */
		if((!(vm->reg->flags & RF_STACK) || t->hasStackRegion(vm)) &&
				(!(vm->reg->flags & RF_TLS) || t->getTLSRegion() == vm)) {
			ssize_t res;
			size_t pageCount,swapped;
			sVMRegion *nvm = vmreg_add(&dst->regtree,vm->reg,vm->virt);
			if(nvm == NULL)
				goto error;
			mutex_aquire(&vm->reg->lock);
			/* TODO ?? better don't share the file; they may have to read in parallel */
			if(vm->reg->flags & RF_SHAREABLE) {
				reg_addTo(vm->reg,dst);
				nvm->reg = vm->reg;
			}
			else {
				nvm->reg = reg_clone(dst,vm->reg);
				if(nvm->reg == NULL) {
					mutex_release(&vm->reg->lock);
					goto error;
				}
			}

			/* remove regions in the free area from the free-map */
			if(vm->virt >= FREE_AREA_BEGIN) {
				if(!vmfree_allocateAt(&dst->freemap,nvm->virt,
						ROUND_PAGE_UP(nvm->reg->byteCount)))
					goto error;
			}

			/* now copy the pages */
			pageCount = BYTES_2_PAGES(nvm->reg->byteCount);
			res = paging_clonePages(src->getPageDir(),dst->getPageDir(),vm->virt,nvm->virt,pageCount,
					vm->reg->flags & RF_SHAREABLE);
			if(res < 0) {
				mutex_release(&vm->reg->lock);
				goto error;
			}
			dst->addOwn(res);

			/* update stats */
			if(vm->reg->flags & RF_SHAREABLE) {
				dst->addShared(reg_pageCount(nvm->reg,&swapped));
				dst->addSwap(swapped);
			}
			/* add frames to copy-on-write, if not shared */
			else {
				uintptr_t virt = nvm->virt;
				for(j = 0; j < pageCount; j++) {
					if(vm->reg->pageFlags[j] & PF_SWAPPED)
						dst->addSwap(1);
					/* not when demand-load or swapping is outstanding since we've not loaded it
					 * from disk yet */
					else if(!(vm->reg->pageFlags[j] & (PF_DEMANDLOAD | PF_SWAPPED))) {
						frameno_t frameNo = paging_getFrameNo(src->getPageDir(),virt);
						/* if not already done, mark as cow for parent */
						if(!(vm->reg->pageFlags[j] & PF_COPYONWRITE)) {
							vm->reg->pageFlags[j] |= PF_COPYONWRITE;
							if(!CopyOnWrite::add(frameNo)) {
								mutex_release(&vm->reg->lock);
								goto error;
							}
							src->addShared(1);
							src->addOwn(-1);
						}
						/* do it always for the child */
						nvm->reg->pageFlags[j] |= PF_COPYONWRITE;
						if(!CopyOnWrite::add(frameNo)) {
							mutex_release(&vm->reg->lock);
							goto error;
						}
						dst->addShared(1);
					}
					virt += PAGE_SIZE;
				}
			}
			mutex_release(&vm->reg->lock);
		}
	}
	vmm_relProc(src);
	vmm_relProc(dst);
	return 0;

error:
	vmm_relProc(src);
	vmm_relProc(dst);
	/* Note that vmm_remove() will undo the cow just for the child; but thats ok since
	 * the parent will get its frame back as soon as it writes to it */
	/* Note also that we don't restore the old frame-counts; for dst it makes no sense because
	 * we'll destroy the process anyway. for src we can't do it because the cow-entries still
	 * exists and therefore its correct. */
	Proc::removeRegions(dstId,true);
	/* no need to free regs, since vmm_remove has already done it */
	return -ENOMEM;
}

int vmm_growStackTo(pid_t pid,sVMRegion *vm,uintptr_t addr) {
	Proc *p = vmm_reqProc(pid);
	ssize_t newPages = 0;
	int res = -ENOMEM;
	addr &= ~(PAGE_SIZE - 1);
	/* report failure if its outside (upper / lower) of the region */
	/* note that we assume here that if a thread has multiple stack-regions, they grow towards
	 * each other */
	if(vm->reg->flags & RF_GROWS_DOWN) {
		if(addr < vm->virt + ROUND_PAGE_UP(vm->reg->byteCount) && addr < vm->virt) {
			res = 0;
			newPages = (vm->virt - addr) / PAGE_SIZE;
		}
	}
	else {
		if(addr >= vm->virt && addr >= vm->virt + ROUND_PAGE_UP(vm->reg->byteCount)) {
			res = 0;
			newPages = ROUND_PAGE_UP(addr - (vm->virt + ROUND_PAGE_UP(vm->reg->byteCount) - 1)) / PAGE_SIZE;
		}
	}

	if(res == 0) {
		/* if its too much, try the next one; if there is none that fits, report failure */
		if(BYTES_2_PAGES(vm->reg->byteCount) + newPages >= MAX_STACK_PAGES - 1)
			res = -ENOMEM;
		/* new pages necessary? */
		else if(newPages > 0) {
			Thread *t = Thread::getRunning();
			if(!t->reserveFrames(newPages)) {
				vmm_relProc(p);
				return -ENOMEM;
			}
			res = vmm_doGrow(p,vm,newPages) != 0 ? 0 : -ENOMEM;
			t->discardFrames();
		}
	}
	vmm_relProc(p);
	return res;
}

size_t vmm_grow(pid_t pid,uintptr_t addr,ssize_t amount) {
	sVMRegion *vm;
	size_t res = 0;
	Proc *p = vmm_reqProc(pid);
	if(!p)
		return 0;
	vm = vmreg_getByAddr(&p->regtree,addr);
	if(vm)
		res = vmm_doGrow(p,vm,amount);
	vmm_relProc(p);
	return res;
}

static size_t vmm_doGrow(Proc *p,sVMRegion *vm,ssize_t amount) {
	size_t oldSize;
	ssize_t res = -ENOMEM;
	uintptr_t oldVirt,virt;
	mutex_aquire(&vm->reg->lock);
	assert((vm->reg->flags & RF_GROWABLE) && !(vm->reg->flags & RF_SHAREABLE));

	/* check whether we've reached the max stack-pages */
	if((vm->reg->flags & RF_STACK) &&
			BYTES_2_PAGES(vm->reg->byteCount) + amount >= MAX_STACK_PAGES - 1) {
		mutex_release(&vm->reg->lock);
		return 0;
	}

	/* resize region */
	oldVirt = vm->virt;
	oldSize = vm->reg->byteCount;
	if(amount != 0) {
		ssize_t pts;
		/* check whether the space is free */
		if(amount > 0) {
			if(vm->reg->flags & RF_GROWS_DOWN) {
				if(vmm_isOccupied(p,oldVirt - amount * PAGE_SIZE,oldVirt)) {
					mutex_release(&vm->reg->lock);
					return 0;
				}
			}
			else {
				uintptr_t end = oldVirt + ROUND_PAGE_UP(vm->reg->byteCount);
				if(vmm_isOccupied(p,end,end + amount * PAGE_SIZE)) {
					mutex_release(&vm->reg->lock);
					return 0;
				}
			}
		}
		if((res = reg_grow(vm->reg,amount)) < 0) {
			mutex_release(&vm->reg->lock);
			return 0;
		}

		/* map / unmap pages */
		if(amount > 0) {
			uint mapFlags = PG_PRESENT;
			if(vm->reg->flags & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			/* if it grows down, pages are added before the existing */
			if(vm->reg->flags & RF_GROWS_DOWN) {
				vm->virt -= amount * PAGE_SIZE;
				virt = vm->virt;
			}
			else
				virt = vm->virt + ROUND_PAGE_UP(oldSize);
			pts = paging_mapTo(p->getPageDir(),virt,NULL,amount,mapFlags);
			if(pts < 0) {
				if(vm->reg->flags & RF_GROWS_DOWN)
					vm->virt += amount * PAGE_SIZE;
				mutex_release(&vm->reg->lock);
				return 0;
			}
			p->addOwn(pts + amount);
			/* remove it from the free area (used by the dynlinker for its data-region only) */
			/* note that we assume here that we get the location at the end of the data-region */
			if(vm->virt >= FREE_AREA_BEGIN)
				vmfree_allocate(&p->freemap,amount * PAGE_SIZE);
		}
		else {
			if(vm->reg->flags & RF_GROWS_DOWN) {
				virt = vm->virt;
				vm->virt += -amount * PAGE_SIZE;
			}
			else
				virt = vm->virt + ROUND_PAGE_UP(vm->reg->byteCount);
			/* give it back to the free area */
			if(vm->virt >= FREE_AREA_BEGIN)
				vmfree_free(&p->freemap,virt,-amount * PAGE_SIZE);
			pts = paging_unmapFrom(p->getPageDir(),virt,-amount,true);
			p->addOwn(-(pts - amount));
			p->addSwap(-res);
		}
	}

	res = (vm->reg->flags & RF_GROWS_DOWN) ? oldVirt : oldVirt + ROUND_PAGE_UP(oldSize);
	mutex_release(&vm->reg->lock);
	return res;
}

void vmm_sprintfRegions(sStringBuffer *buf,pid_t pid) {
	Proc *p = vmm_reqProc(pid);
	if(p) {
		sVMRegion *vm;
		size_t c = 0;
		for(vm = p->regtree.begin; vm != NULL; vm = vm->next) {
			if(c)
				prf_sprintf(buf,"\n");
			mutex_aquire(&vm->reg->lock);
			prf_sprintf(buf,"VMRegion (%p .. %p):\n",vm->virt,vm->virt + vm->reg->byteCount - 1);
			reg_sprintf(buf,vm->reg,vm->virt);
			mutex_release(&vm->reg->lock);
			c++;
		}
		vmm_relProc(p);
	}
}

void vmm_sprintfMaps(sStringBuffer *buf,pid_t pid) {
	Proc *p = vmm_reqProc(pid);
	if(p) {
		sVMRegion *vm;
		for(vm = p->regtree.begin; vm != NULL; vm = vm->next) {
			mutex_aquire(&vm->reg->lock);
			prf_sprintf(buf,"%-24s %p - %p (%5zuK) %c%c%c%c",vmm_getRegName(p,vm),vm->virt,
					vm->virt + vm->reg->byteCount - 1,vm->reg->byteCount / K,
					(vm->reg->flags & RF_WRITABLE) ? 'w' : '-',
					(vm->reg->flags & RF_EXECUTABLE) ? 'x' : '-',
					(vm->reg->flags & RF_GROWABLE) ? 'g' : '-',
					(vm->reg->flags & RF_SHAREABLE) ? 's' : '-');
			mutex_release(&vm->reg->lock);
			prf_sprintf(buf,"\n");
		}
		vmm_relProc(p);
	}
}

static const char *vmm_getRegName(Proc *p,const sVMRegion *vm) {
	const char *name = "";
	if(vm->virt == p->dataAddr)
		name = "data";
	else if(vm->reg->flags & RF_STACK)
		name = "stack";
	else if(vm->reg->flags & RF_TLS)
		name = "tls";
	else if(vm->reg->flags & RF_NOFREE)
		name = "phys";
	else if(vm->reg->file)
		name = vfs_getPath(vm->reg->file);
	else if(p->getEntryPoint() >= vm->virt && p->getEntryPoint() < vm->virt + vm->reg->byteCount)
		name = p->getCommand();
	else
		name = "???";
	return name;
}

void vmm_printShort(pid_t pid,const char *prefix) {
	Proc *p = Proc::getByPid(pid);
	if(p) {
		sVMRegion *vm;
		for(vm = p->regtree.begin; vm != NULL; vm = vm->next) {
			vid_printf("%s%-24s %p - %p (%5zuK) ",prefix,vmm_getRegName(p,vm),vm->virt,
					vm->virt + vm->reg->byteCount - 1,vm->reg->byteCount / K);
			reg_printFlags(vm->reg);
			vid_printf("\n");
		}
	}
}

void vmm_print(pid_t pid) {
	Proc *p = Proc::getByPid(pid);
	if(p) {
		size_t c = 0;
		sVMRegion *vm;
		vid_printf("Regions of proc %d (%s)\n",pid,p->getCommand());
		for(vm = p->regtree.begin; vm != NULL; vm = vm->next) {
			if(c)
				vid_printf("\n");
			vid_printf("VMRegion (%p .. %p):\n",vm->virt,vm->virt + vm->reg->byteCount - 1);
			reg_print(vm->reg,vm->virt);
			c++;
		}
		if(c == 0)
			vid_printf("- no regions -\n");
	}
}

static bool vmm_demandLoad(Proc *p,sVMRegion *vm,uintptr_t addr) {
	bool res = false;

	/* calculate the number of bytes to load and zero */
	size_t loadCount = 0, zeroCount;
	if(addr - vm->virt < vm->reg->loadCount)
		loadCount = MIN(PAGE_SIZE,vm->reg->loadCount - (addr - vm->virt));
	else
		res = true;
	zeroCount = MIN(PAGE_SIZE,vm->reg->byteCount - (addr - vm->virt)) - loadCount;

	/* load from file */
	if(loadCount)
		res = vmm_loadFromFile(p,vm,addr,loadCount);

	/* zero the rest, if necessary */
	if(res && zeroCount) {
		/* do the memclear before the mapping to ensure that it's ready when the first CPU sees it */
		frameno_t frame = loadCount ? paging_getFrameNo(Proc::getCurPageDir(),addr)
									: Thread::getRunning()->getFrame();
		uintptr_t frameAddr = paging_getAccess(frame);
		memclear((void*)(frameAddr + loadCount),zeroCount);
		paging_removeAccess();
		/* if the pages weren't present so far, map them into every process that has this region */
		if(!loadCount) {
			sSLNode *n;
			uint mapFlags = PG_PRESENT;
			if(vm->reg->flags & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			for(n = sll_begin(&vm->reg->procs); n != NULL; n = n->next) {
				Proc *mp = (Proc*)n->data;
				/* the region may be mapped to a different virtual address */
				sVMRegion *mpreg = vmreg_getByReg(&mp->regtree,vm->reg);
				/* can't fail */
				assert(paging_mapTo(mp->getPageDir(),mpreg->virt + (addr - vm->virt),&frame,1,mapFlags) == 0);
				mp->addShared(1);
			}
		}
	}
	return res;
}

static bool vmm_loadFromFile(Proc *p,sVMRegion *vm,uintptr_t addr,size_t loadCount) {
	int err;
	pid_t pid = p->getPid();
	void *tempBuf;
	sSLNode *n;
	frameno_t frame;
	uint mapFlags;

	/* note that we currently ignore that the file might have changed in the meantime */
	if((err = vfs_seek(pid,vm->reg->file,vm->reg->offset + (addr - vm->virt),SEEK_SET)) < 0)
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
	err = vfs_readFile(pid,vm->reg->file,tempBuf,loadCount);
	if(err != (ssize_t)loadCount) {
		if(err >= 0)
			err = -ENOMEM;
		goto errorFree;
	}

	/* copy into frame */
	frame = paging_demandLoad(tempBuf,loadCount,vm->reg->flags);

	/* free resources not needed anymore */
	Thread::remHeapAlloc(tempBuf);
	Cache::free(tempBuf);

	/* map into all pagedirs */
	mapFlags = PG_PRESENT;
	if(vm->reg->flags & RF_WRITABLE)
		mapFlags |= PG_WRITABLE;
	if(vm->reg->flags & RF_EXECUTABLE)
		mapFlags |= PG_EXECUTABLE;
	for(n = sll_begin(&vm->reg->procs); n != NULL; n = n->next) {
		Proc *mp = (Proc*)n->data;
		/* the region may be mapped to a different virtual address */
		sVMRegion *mpreg = vmreg_getByReg(&mp->regtree,vm->reg);
		/* can't fail */
		assert(paging_mapTo(mp->getPageDir(),mpreg->virt + (addr - vm->virt),&frame,1,mapFlags) == 0);
		if(vm->reg->flags & RF_SHAREABLE)
			mp->addShared(1);
		else
			mp->addOwn(1);
	}
	return true;

errorFree:
	Thread::remHeapAlloc(tempBuf);
	Cache::free(tempBuf);
error:
	log_printf("Demandload page @ %p for proc %s: %s (%d)\n",addr,p->getCommand(),strerror(-err),err);
	return false;
}

static sRegion *vmm_getLRURegion(void) {
	sVMRegion *vm;
	sRegion *lru = NULL;
	time_t ts = UINT_MAX;
	Proc *p;
	sVMRegTree *tree = vmreg_reqTree();
	for(; tree != NULL; tree = tree->next) {
		/* the disk-driver is not swappable */
		if(tree->pid == DISK_PID)
			continue;

		/* same as below; we have to try to aquire the mutex, otherwise we risk a deadlock */
		p = vmm_tryReqProc(tree->pid);
		if(p) {
			for(vm = tree->begin; vm != NULL; vm = vm->next) {
				size_t j,count = 0,pages;
				if((vm->reg->flags & RF_NOFREE) || vm->reg->timestamp >= ts)
					continue;

				/* we can't block here because otherwise we risk a deadlock. suppose that fs has to
				 * swap out to get more memory. if we want to demand-load something before this operation
				 * is finished and lock the region for that, the swapper will find this region at this
				 * place locked. so we have to skip it in this case to be able to continue. */
				if(mutex_tryAquire(&vm->reg->lock)) {
					pages = BYTES_2_PAGES(vm->reg->byteCount);
					for(j = 0; j < pages; j++) {
						if(!(vm->reg->pageFlags[j] & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD))) {
							count++;
							break;
						}
					}
					if(count > 0) {
						if(lru)
							mutex_release(&lru->lock);
						ts = vm->reg->timestamp;
						lru = vm->reg;
					}
					else
						mutex_release(&vm->reg->lock);
				}
			}
			vmm_relProc(p);
		}
	}
	vmreg_relTree();
	return lru;
}

static ssize_t vmm_getPgIdxForSwap(const sRegion *reg) {
	size_t i,pages = BYTES_2_PAGES(reg->byteCount);
	size_t index,count = 0;
	for(i = 0; i < pages; i++) {
		if(!(reg->pageFlags[i] & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD)))
			count++;
	}
	if(count > 0) {
		index = util_rand() % count;
		for(i = 0; i < pages; i++) {
			if(!(reg->pageFlags[i] & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD))) {
				if(index-- == 0)
					return i;
			}
		}
	}
	return -1;
}

static void vmm_setSwappedOut(sRegion *reg,size_t index) {
	sSLNode *n;
	uintptr_t offset = index * PAGE_SIZE;
	reg->pageFlags[index] |= PF_SWAPPED;
	for(n = sll_begin(&reg->procs); n != NULL; n = n->next) {
		Proc *mp = (Proc*)n->data;
		/* the region may be mapped to a different virtual address */
		sVMRegion *mpreg = vmreg_getByReg(&mp->regtree,reg);
		/* can't fail */
		assert(paging_mapTo(mp->getPageDir(),mpreg->virt + offset,NULL,1,0) == 0);
		if(reg->flags & RF_SHAREABLE)
			mp->addShared(-1);
		else
			mp->addOwn(-1);
		mp->addSwap(1);
	}
}

static void vmm_setSwappedIn(sRegion *reg,size_t index,frameno_t frameNo) {
	sSLNode *n;
	uintptr_t offset = index * PAGE_SIZE;
	uint flags = PG_PRESENT;
	if(reg->flags & RF_WRITABLE)
		flags |= PG_WRITABLE;
	if(reg->flags & RF_EXECUTABLE)
		flags |= PG_EXECUTABLE;
	reg->pageFlags[index] &= ~PF_SWAPPED;
	reg_setSwapBlock(reg,index,0);
	for(n = sll_begin(&reg->procs); n != NULL; n = n->next) {
		Proc *mp = (Proc*)n->data;
		/* the region may be mapped to a different virtual address */
		sVMRegion *mpreg = vmreg_getByReg(&mp->regtree,reg);
		/* can't fail */
		assert(paging_mapTo(mp->getPageDir(),mpreg->virt + offset,&frameNo,1,flags) == 0);
		if(reg->flags & RF_SHAREABLE)
			mp->addShared(1);
		else
			mp->addOwn(1);
		mp->addSwap(-1);
	}
}

static uintptr_t vmm_findFreeStack(Proc *p,size_t byteCount,A_UNUSED ulong rflags) {
	uintptr_t addr,end;
	size_t size = ROUND_PAGE_UP(byteCount);
	sVMRegion *dataReg;
	/* leave a gap between the stacks as a guard */
	if(byteCount > (MAX_STACK_PAGES - 1) * PAGE_SIZE)
		return 0;
#if STACK_AREA_GROWS_DOWN
	/* find end address */
	dataReg = vmreg_getByAddr(&p->regtree,p->dataAddr);
	if(dataReg)
		end = dataReg->virt + ROUND_PAGE_UP(dataReg->reg->byteCount);
	else
		end = vmm_getFirstUsableAddr(p);
	/* determine start address */
	if(p->freeStackAddr != 0)
		addr = p->freeStackAddr;
	else
		addr = STACK_AREA_END;
	for(; addr > end; addr -= MAX_STACK_PAGES * PAGE_SIZE) {
		uintptr_t start = addr - size;
		/* in this case it is not necessary to use vmm_isOccupied() because all stack-regions
		 * have a fixed max-size and there is no other region that can be in that area. so its
		 * enough to test whether in this area is a region or not */
		if(vmreg_getByAddr(&p->regtree,start) == NULL) {
			p->freeStackAddr = addr - MAX_STACK_PAGES * PAGE_SIZE;
			return start;
		}
	}
#else
	for(addr = STACK_AREA_BEGIN; addr < STACK_AREA_END; addr += MAX_STACK_PAGES * PAGE_SIZE) {
		if(vmm_isOccupied(p,addr,addr + (MAX_STACK_PAGES - 1) * PAGE_SIZE) == NULL) {
			if(rflags & RF_GROWS_DOWN)
				return addr + (MAX_STACK_PAGES - 1) * PAGE_SIZE - ROUND_PAGE_UP(byteCount);
			return addr;
		}
	}
#endif
	return 0;
}

static sVMRegion *vmm_isOccupied(Proc *p,uintptr_t start,uintptr_t end) {
	sVMRegion *vm;
	for(vm = p->regtree.begin; vm != NULL; vm = vm->next) {
		uintptr_t rstart = vm->virt;
		uintptr_t rend = vm->virt + ROUND_PAGE_UP(vm->reg->byteCount);
		if(OVERLAPS(rstart,rend,start,end))
			return vm;
	}
	return NULL;
}

static uintptr_t vmm_getFirstUsableAddr(Proc *p) {
	uintptr_t addr = 0;
	sVMRegion *vm;
	for(vm = p->regtree.begin; vm != NULL; vm = vm->next) {
		if(!(vm->reg->flags & RF_STACK) && vm->virt < FREE_AREA_BEGIN &&
				vm->virt + vm->reg->byteCount > addr) {
			addr = vm->virt + vm->reg->byteCount;
		}
	}
	return ROUND_PAGE_UP(addr);
}

static Proc *vmm_reqProc(pid_t pid) {
	return Proc::request(pid,PLOCK_REGIONS);
}

static Proc *vmm_tryReqProc(pid_t pid) {
	return Proc::tryRequest(pid,PLOCK_REGIONS);
}

static void vmm_relProc(Proc *p) {
	Proc::release(p,PLOCK_REGIONS);
}
