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
#define REG(p,i)			(((sVMRegion**)(p)->regions)[(i)])
#define ROUNDUP(bytes)		(((bytes) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

static sRegion *vmm_getLRURegion(void);
static ssize_t vmm_getPgIdxForSwap(const sRegion *reg);
static void vmm_setSwappedOut(sRegion *reg,size_t index);
static void vmm_setSwappedIn(sRegion *reg,size_t index,frameno_t frameNo);
static vmreg_t vmm_getRegionOf(sProc *p,uintptr_t addr);
static vmreg_t vmm_getRNoByRegion(sProc *p,const sRegion *reg);
static bool vmm_doPagefault(uintptr_t addr,sProc *p,sVMRegion *vm,bool write);
static void vmm_doRemove(sProc *p,vmreg_t reg);
static size_t vmm_doGrow(sProc *p,vmreg_t reg,ssize_t amount);
static void vmm_unsetRegion(sProc *p,vmreg_t rno);
static bool vmm_demandLoad(sProc *p,sVMRegion *vm,uintptr_t addr);
static bool vmm_loadFromFile(sProc *p,sVMRegion *vm,uintptr_t addr,size_t loadCount);
static vmreg_t vmm_findRegIndex(sProc *p,bool text);
static uintptr_t vmm_findFreeStack(sProc *p,size_t byteCount,ulong rflags);
static sVMRegion *vmm_isOccupied(sProc *p,uintptr_t start,uintptr_t end);
static uintptr_t vmm_getFirstUsableAddr(sProc *p,bool textNData);
static uintptr_t vmm_findFreeAddr(sProc *p,size_t byteCount);
static bool vmm_extendRegions(sProc *p,size_t i);
static sVMRegion *vmm_alloc(void);
static void vmm_free(sVMRegion *vm,const sProc *p);
static int vmm_getAttr(sProc *p,uint type,size_t bCount,ulong *pgFlags,ulong *flags,
		uintptr_t *virt,vmreg_t *rno);

/* a linked list of all regions */
static sVMRegion *regions;
static klock_t regLock;
static uint8_t buffer[PAGE_SIZE];

void vmm_init(void) {
	/* nothing to do */
}

uintptr_t vmm_addPhys(pid_t pid,uintptr_t *phys,size_t bCount,size_t align) {
	vmreg_t reg;
	sVMRegion *vm;
	ssize_t res;
	sProc *p;
	size_t i,pages = BYTES_2_PAGES(bCount);
	frameno_t *frames = (frameno_t*)cache_alloc(sizeof(frameno_t) * pages);
	if(frames == NULL)
		return 0;

	/* if *phys is not set yet, we should allocate physical contiguous memory */
	if(*phys == 0) {
		ssize_t first = pmem_allocateContiguous(pages,align / PAGE_SIZE);
		if(first < 0)
			return 0;
		for(i = 0; i < pages; i++)
			frames[i] = first + i;
	}
	/* otherwise use the specified one */
	else {
		for(i = 0; i < pages; i++) {
			frames[i] = *phys / PAGE_SIZE;
			*phys += PAGE_SIZE;
		}
	}

	/* create region */
	reg = vmm_add(pid,NULL,0,bCount,bCount,*phys ? REG_DEVICE : REG_PHYS);
	if(reg < 0) {
		if(!*phys)
			pmem_freeContiguous(frames[0],pages);
		cache_free(frames);
		return 0;
	}

	/* map memory */
	p = proc_request(pid,PLOCK_REGIONS);
	if(!p) {
		vmm_remove(pid,reg);
		cache_free(frames);
		return 0;
	}
	vm = REG(p,reg);
	res = paging_mapTo(&p->pagedir,vm->virt,frames,pages,PG_PRESENT | PG_WRITABLE);
	if(res < 0) {
		proc_release(p,PLOCK_REGIONS);
		vmm_remove(pid,reg);
		cache_free(frames);
		return 0;
	}
	if(*phys) {
		/* the page-tables are ours, the pages may be used by others, too */
		p->ownFrames += res;
		p->sharedFrames += pages;
	}
	else {
		/* its our own mem; store physical address for the caller */
		p->ownFrames += res;
		*phys = frames[0] * PAGE_SIZE;
	}
	proc_release(p,PLOCK_REGIONS);
	cache_free(frames);
	return vm->virt;
}

vmreg_t vmm_add(pid_t pid,const sBinDesc *bin,off_t binOffset,size_t bCount,size_t lCount,uint type) {
	sRegion *reg;
	sVMRegion *vm;
	vmreg_t rno;
	int res;
	uintptr_t virt;
	ulong pgFlags,flags;
	sProc *p;

	/* for text and shared-library-text: try to find another process with that text */
	if(bin && (type == REG_TEXT || type == REG_SHLIBTEXT)) {
		vmreg_t prno;
		pid_t binowner = proc_getProcWithBin(bin,&prno);
		if(binowner != INVALID_PID)
			return vmm_join(binowner,prno,pid);
	}

	/* get the attributes of the region (depending on type) */
	p = proc_request(pid,PLOCK_REGIONS);
	if(!p)
		return -ESRCH;
	if((res = vmm_getAttr(p,type,bCount,&pgFlags,&flags,&virt,&rno)) < 0)
		goto errProc;
	/* no demand-loading if the binary isn't present */
	if(bin == NULL)
		pgFlags &= ~PF_DEMANDLOAD;

	/* create region */
	res = -ENOMEM;
	reg = reg_create(bin,binOffset,bCount,lCount,pgFlags,flags);
	if(!reg)
		goto errProc;
	if(!reg_addTo(reg,p))
		goto errReg;
	vm = vmm_alloc();
	if(vm == NULL)
		goto errAdd;
	vm->binFile = -1;
	vm->reg = reg;
	vm->proc = p;
	vm->virt = virt;
	REG(p,rno) = vm;

	/* map into process */
	if(type != REG_DEVICE && type != REG_PHYS) {
		ssize_t pts;
		uint mapFlags = 0;
		size_t pageCount = BYTES_2_PAGES(vm->reg->byteCount);
		if(!(pgFlags & PF_DEMANDLOAD)) {
			mapFlags |= PG_PRESENT;
			if(flags & RF_EXECUTABLE)
				mapFlags |= PG_EXECUTABLE;
		}
		if(flags & RF_WRITABLE)
			mapFlags |= PG_WRITABLE;
		pts = paging_mapTo(&p->pagedir,virt,NULL,pageCount,mapFlags);
		if(pts < 0)
			goto errMap;
		if(flags & RF_SHAREABLE)
			p->sharedFrames += pageCount;
		else
			p->ownFrames += pageCount;
		p->ownFrames += pts;
	}
	proc_release(p,PLOCK_REGIONS);

#if DISABLE_DEMLOAD
	if(type != REG_DEVICE && type != REG_PHYS) {
		size_t i;
		for(i = 0; i < vm->reg->pfSize; i++)
			vmm_pagefault(vm->virt + i * PAGE_SIZE);
	}
#endif
	return rno;

errMap:
	vmm_free(vm,p);
	vmm_unsetRegion(p,rno);
errAdd:
	reg_remFrom(reg,p);
errReg:
	reg_destroy(reg);
errProc:
	proc_release(p,PLOCK_REGIONS);
	return res;
}

int vmm_setRegProt(pid_t pid,uintptr_t addr,ulong flags) {
	size_t i,pgcount;
	sSLNode *n;
	sVMRegion *vmreg;
	vmreg_t rno;
	sProc *p;
	int res = -EPERM;
	if(!(flags & RF_WRITABLE) && flags != 0)
		return -EINVAL;
	p = proc_request(pid,PLOCK_REGIONS);
	if(!p)
		return -ESRCH;
	rno = vmm_getRegionOf(p,addr);
	if(rno == -1) {
		proc_release(p,PLOCK_REGIONS);
		return -ENXIO;
	}
	vmreg = REG(p,rno);
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
	vmreg->reg->flags &= ~RF_WRITABLE;
	vmreg->reg->flags |= flags;
	/* change mapping */
	for(n = sll_begin(vmreg->reg->procs); n != NULL; n = n->next) {
		sProc *mp = (sProc*)n->data;
		/* the region may be mapped to a different virtual address */
		vmreg_t mprno = vmm_getRNoByRegion(mp,vmreg->reg);
		sVMRegion *mpreg = REG(mp,mprno);
		assert(mprno != -1);
		for(i = 0; i < pgcount; i++) {
			/* determine flags; we can't always mark it present.. */
			uint mapFlags = PG_KEEPFRM;
			if(!(vmreg->reg->pageFlags[i] & (PF_DEMANDLOAD | PF_SWAPPED)))
				mapFlags |= PG_PRESENT;
			if(vmreg->reg->flags & RF_EXECUTABLE)
				mapFlags |= PG_EXECUTABLE;
			if(flags & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			/* can't fail because of PG_KEEPFRM and because the page-table is always present */
			assert(paging_mapTo(&mp->pagedir,mpreg->virt + i * PAGE_SIZE,NULL,1,mapFlags) == 0);
		}
	}
	res = 0;
error:
	mutex_release(&vmreg->reg->lock);
	proc_release(p,PLOCK_REGIONS);
	return res;
}

void vmm_swapOut(pid_t pid,file_t file,size_t count) {
	while(count > 0) {
		sRegion *reg = vmm_getLRURegion();
		if(reg == NULL)
			util_panic("No pages to swap out");

		/* the list of processes may have changed in the meantime; we can't swap out in this case */
		while(count > 0) {
			frameno_t frameNo;
			ulong block;
			ssize_t index;
			sProc *proc;
			vmreg_t rno;
			sVMRegion *vmreg;

			/* get page to swap out */
			index = vmm_getPgIdxForSwap(reg);
			if(index < 0)
				break;

			/* get VM-region of first process */
			proc = (sProc*)sll_get(reg->procs,0);
			rno = vmm_getRNoByRegion(proc,reg);
			vmreg = REG(proc,rno);

			/* find swap-block */
			block = swmap_alloc();
			assert(block != INVALID_BLOCK);

#if DEBUG_SWAP
			sSLNode *n;
			log_printf("OUT: %d of region %x (block %d)\n",index,vmreg->reg,block);
			for(n = sll_begin(reg->procs); n != NULL; n = n->next) {
				sProc *mp = (sProc*)n->data;
				vmreg_t mprno = vmm_getRNoByRegion(mp->pid,reg);
				sVMRegion *mpreg = REG(mp,mprno);
				log_printf("\tProcess %d:%s -> page %p\n",mp->pid,mp->command,
						mpreg->virt + index * PAGE_SIZE);
			}
			log_printf("\n");
#endif

			/* get the frame first, because the page has to be present */
			frameNo = paging_getFrameNo(&proc->pagedir,vmreg->virt + index * PAGE_SIZE);
			/* unmap the page in all processes and ensure that all CPUs have flushed their TLB */
			/* this way we know that nobody can still access the page; if someone tries, he will
			 * cause a page-fault and will wait until we release the region-mutex */
			vmm_setSwappedOut(reg,index);
			reg_setSwapBlock(reg,index,block);
			smp_ensureTLBFlushed();

			/* copy to a temporary buffer because we can't use the temp-area when switching threads */
			paging_copyFromFrame(frameNo,buffer);
			pmem_free(frameNo,false);

			/* write out on disk */
			assert(vfs_seek(pid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
			assert(vfs_writeFile(pid,file,buffer,PAGE_SIZE) == PAGE_SIZE);

			count--;
		}
		mutex_release(&reg->lock);
	}
}

bool vmm_swapIn(pid_t pid,file_t file,sThread *t,uintptr_t addr) {
	sVMRegion *vmreg;
	frameno_t frame;
	ulong block;
	size_t index;
	vmreg_t rno = vmm_getRegionOf(t->proc,addr);
	if(rno == -1)
		return false;
	vmreg = REG(t->proc,rno);
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
	for(n = sll_begin(vmreg->reg->procs); n != NULL; n = n->next) {
		sProc *mp = (sProc*)n->data;
		vmreg_t mprno = vmm_getRNoByRegion(mp->pid,vmreg->reg);
		sVMRegion *mpreg = REG(mp,mprno);
		log_printf("\tProcess %d:%s -> page %p\n",mp->pid,mp->command,
				mpreg->virt + index * PAGE_SIZE);
	}
	log_printf("\n");
#endif

	/* read into buffer (note that we can use the same for swap-in and swap-out because its both
	 * done by the swapper-thread) */
	assert(vfs_seek(pid,file,block * PAGE_SIZE,SEEK_SET) >= 0);
	assert(vfs_readFile(pid,file,buffer,PAGE_SIZE) == PAGE_SIZE);

	/* copy into a new frame */
	frame = thread_getFrame(t);
	paging_copyToFrame(frame,buffer);

	/* mark as not-swapped and map into all affected processes */
	vmm_setSwappedIn(vmreg->reg,index,frame);
	/* free swap-block */
	swmap_free(block);
	return true;
}

void vmm_setTimestamp(const sThread *t,uint64_t timestamp) {
	size_t i;
	size_t tls = thread_getTLSRegion(t);
	for(i = 0; i < t->proc->regSize; i++) {
		sVMRegion *vm = REG(t->proc,i);
		if(vm) {
			/* for stack and tls: just mark the ones of the given thread used */
			if((!(vm->reg->flags & RF_TLS) || i == tls) &&
				(!(vm->reg->flags & RF_STACK) || thread_hasStackRegion(t,i))) {
				vm->reg->timestamp = timestamp;
			}
		}
	}
}

bool vmm_exists(pid_t pid,vmreg_t reg) {
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	bool res = false;
	if(p) {
		res = reg >= 0 && reg < (vmreg_t)p->regSize && REG(p,reg);
		proc_release(p,PLOCK_REGIONS);
	}
	return res;
}

vmreg_t vmm_getDLDataReg(pid_t pid) {
	vmreg_t res = -1;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(p) {
		size_t i;
		for(i = 0; i < p->regSize; i++) {
			sVMRegion *vm = REG(p,i);
			if(vm && vm->reg->flags == (RF_WRITABLE | RF_GROWABLE) &&
					vm->virt >= FREE_AREA_BEGIN) {
				res = i;
				break;
			}
		}
		proc_release(p,PLOCK_REGIONS);
	}
	return res;
}

void vmm_getMemUsageOf(pid_t pid,size_t *own,size_t *shared,size_t *swapped) {
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	*own = p->ownFrames;
	*shared = p->sharedFrames;
	*swapped = p->swapped;
	proc_release(p,PLOCK_REGIONS);
}

float vmm_getMemUsage(pid_t pid,size_t *pages) {
	size_t i;
	float rpages = 0;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	*pages = 0;
	if(p) {
		for(i = 0; i < p->regSize; i++) {
			sVMRegion *vm = REG(p,i);
			if(vm) {
				size_t j,count = 0;
				size_t pageCount = BYTES_2_PAGES(vm->reg->byteCount);
				for(j = 0; j < pageCount; j++) {
					if(!(vm->reg->pageFlags[j] & (PF_SWAPPED | PF_DEMANDLOAD | PF_COPYONWRITE)))
						count++;
				}
				*pages += pageCount;
				rpages += (float)count / reg_refCount(vm->reg);
			}
		}
		proc_release(p,PLOCK_REGIONS);
	}
	return rpages;
}

sVMRegion *vmm_getRegion(pid_t pid,vmreg_t rno) {
	/* FIXME remove this function */
	sProc *p = proc_getByPid(pid);
	return p ? REG(p,rno) : NULL;
}

bool vmm_getRegRange(pid_t pid,vmreg_t reg,uintptr_t *start,uintptr_t *end,bool locked) {
	bool res = false;
	sProc *p = locked ? proc_request(pid,PLOCK_REGIONS) : proc_getByPid(pid);
	if(p) {
		sVMRegion *vm = REG(p,reg);
		assert(reg >= 0 && reg < (vmreg_t)p->regSize);
		if(vm) {
			if(start)
				*start = vm->virt;
			if(end)
				*end = vm->virt + vm->reg->byteCount;
			res = true;
		}
		if(locked)
			proc_release(p,PLOCK_REGIONS);
	}
	return res;
}

vmreg_t vmm_hasBinary(pid_t pid,const sBinDesc *bin) {
	vmreg_t res = -1;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(!p)
		return -1;
	if(p->regSize > 0 && p->regions) {
		size_t i;
		for(i = 0; i < p->regSize; i++) {
			sVMRegion *vm = REG(p,i);
			/* if its shareable and the binary fits, return region-number */
			if(vm && (vm->reg->flags & RF_SHAREABLE) &&
					vm->reg->binary.modifytime == bin->modifytime &&
					vm->reg->binary.ino == bin->ino && vm->reg->binary.dev == bin->dev) {
				res = i;
				break;
			}
		}
	}
	proc_release(p,PLOCK_REGIONS);
	return res;
}

bool vmm_pagefault(uintptr_t addr,bool write) {
	sProc *p;
	sThread *t = thread_getRunning();
	sVMRegion *vm;
	bool res = false;
	vmreg_t rno;

	/* we can swap here; note that we don't need page-tables in this case, they're always present */
	if(!thread_reserveFrames(t,1))
		return false;

	p = proc_request(t->proc->pid,PLOCK_REGIONS);
	rno = vmm_getRegionOf(p,addr);
	if(rno < 0) {
		proc_release(p,PLOCK_REGIONS);
		thread_discardFrames(t);
		return false;
	}
	vm = REG(p,rno);
	res = vmm_doPagefault(addr,p,vm,write);
	proc_release(p,PLOCK_REGIONS);
	thread_discardFrames(t);
	return res;
}

static bool vmm_doPagefault(uintptr_t addr,sProc *p,sVMRegion *vm,bool write) {
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
		res = pmem_swapIn(addr);
	else if(*flags & PF_COPYONWRITE) {
		frameno_t frameNumber = paging_getFrameNo(&p->pagedir,addr);
		size_t frmCount = cow_pagefault(p->pid,addr,frameNumber);
		p->ownFrames += frmCount;
		p->sharedFrames -= frmCount;
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
	size_t i;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(p) {
		for(i = 0; i < p->regSize; i++) {
			sVMRegion *vm = REG(p,i);
			if(vm && (!(vm->reg->flags & RF_STACK) || remStack))
				vmm_doRemove(p,i);
		}
		proc_release(p,PLOCK_REGIONS);
	}
}

void vmm_remove(pid_t pid,vmreg_t reg) {
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(p) {
		vmm_doRemove(p,reg);
		proc_release(p,PLOCK_REGIONS);
	}
}

static void vmm_doRemove(sProc *p,vmreg_t reg) {
	size_t i = 0;
	sVMRegion *vm = REG(p,reg);
	size_t pcount;
	mutex_aquire(&vm->reg->lock);
	pcount = BYTES_2_PAGES(vm->reg->byteCount);
	assert(p && reg < (vmreg_t)p->regSize && vm != NULL);
	assert(reg_remFrom(vm->reg,p));
	if(reg_refCount(vm->reg) == 0) {
		uintptr_t virt = vm->virt;
		/* remove us from cow and unmap the pages (and free frames, if necessary) */
		for(i = 0; i < pcount; i++) {
			ssize_t pts;
			bool freeFrame = !(vm->reg->flags & RF_NOFREE);
			if(vm->reg->pageFlags[i] & PF_COPYONWRITE) {
				bool foundOther;
				frameno_t frameNo = paging_getFrameNo(&p->pagedir,virt);
				/* we can free the frame if there is no other user */
				p->sharedFrames -= cow_remove(p->pid,frameNo,&foundOther);
				freeFrame = !foundOther;
				/* if we'll free the frame with unmap we will substract 1 too much because
				 * we don't own the frame */
				if(freeFrame)
					p->ownFrames++;
			}
			if(vm->reg->pageFlags[i] & PF_SWAPPED)
				p->swapped--;
			pts = paging_unmapFrom(&p->pagedir,virt,1,freeFrame);
			if(freeFrame) {
				if(vm->reg->flags & RF_SHAREABLE)
					p->sharedFrames--;
				else
					p->ownFrames--;
			}
			p->ownFrames -= pts;
			virt += PAGE_SIZE;
		}
		/* now destroy region */
		mutex_release(&vm->reg->lock);
		reg_destroy(vm->reg);
	}
	else {
		size_t swapped;
		/* no free here, just unmap */
		ssize_t pts = paging_unmapFrom(&p->pagedir,vm->virt,pcount,false);
		/* in this case its always a shared region because otherwise there wouldn't be other users */
		/* so we have to substract the present content-frames from the shared ones,
		 * and the ptables from ours */
		p->sharedFrames -= reg_pageCount(vm->reg,&swapped);
		p->ownFrames -= pts;
		p->swapped -= swapped;
		mutex_release(&vm->reg->lock);
	}
	vmm_free(vm,p);
	vmm_unsetRegion(p,reg);
}

static void vmm_unsetRegion(sProc *p,vmreg_t rno) {
	size_t i,c = 0;
	REG(p,rno) = NULL;

	/* check whether all regions are NULL */
	for(i = 0; i < p->regSize; i++) {
		if(REG(p,i)) {
			c++;
			break;
		}
	}
	/* free regions, if all have been removed */
	if(c == 0) {
		cache_free(p->regions);
		p->regSize = 0;
		p->regions = NULL;
	}
}

vmreg_t vmm_join(pid_t srcId,vmreg_t rno,pid_t dstId) {
	sProc *src,*dst;
	sVMRegion *vm,*nvm;
	vmreg_t nrno;
	ssize_t res;
	size_t pageCount,swapped;
	if(srcId == dstId)
		return -EINVAL;

	src = proc_request(srcId,PLOCK_REGIONS);
	dst = proc_request(dstId,PLOCK_REGIONS);
	if(!src || !dst) {
		if(dst)
			proc_release(dst,PLOCK_REGIONS);
		if(src)
			proc_release(src,PLOCK_REGIONS);
		return -ESRCH;
	}

	vm = REG(src,rno);
	mutex_aquire(&vm->reg->lock);
	assert(rno >= 0 && rno < (vmreg_t)src->regSize && vm != NULL);
	assert(vm->reg->flags & RF_SHAREABLE);
	nvm = vmm_alloc();
	if(nvm == NULL)
		goto errProc;

	nvm->binFile = -1;
	nvm->reg = vm->reg;
	nvm->proc = dst;
	if(rno == RNO_TEXT)
		nvm->virt = TEXT_BEGIN;
	else
		nvm->virt = vmm_findFreeAddr(dst,vm->reg->byteCount);
	if(nvm->virt == 0)
		goto errVmm;
	if(!reg_addTo(vm->reg,dst))
		goto errVmm;
	nrno = vmm_findRegIndex(dst,rno == RNO_TEXT);
	if(nrno < 0)
		goto errRem;
	REG(dst,nrno) = nvm;

	/* shared, so content-frames to shared, ptables to own */
	pageCount = BYTES_2_PAGES(vm->reg->byteCount);
	res = paging_clonePages(&src->pagedir,&dst->pagedir,vm->virt,nvm->virt,pageCount,true);
	if(res < 0)
		goto errMap;
	dst->sharedFrames += reg_pageCount(vm->reg,&swapped);
	dst->ownFrames += res;
	dst->swapped += swapped;
	mutex_release(&vm->reg->lock);
	proc_release(dst,PLOCK_REGIONS);
	proc_release(src,PLOCK_REGIONS);
	return nrno;

errMap:
	vmm_unsetRegion(dst,nrno);
errRem:
	reg_remFrom(vm->reg,dst);
errVmm:
	vmm_free(nvm,dst);
errProc:
	mutex_release(&vm->reg->lock);
	proc_release(dst,PLOCK_REGIONS);
	proc_release(src,PLOCK_REGIONS);
	return -ENOMEM;
}

int vmm_cloneAll(pid_t dstId) {
	size_t i,j;
	sThread *t = thread_getRunning();
	sProc *dst = proc_request(dstId,PLOCK_REGIONS);
	sProc *src = proc_request(proc_getRunning(),PLOCK_REGIONS);
	assert(dst->regions == NULL && dst->regSize == 0);
	dst->swapped = 0;
	dst->sharedFrames = 0;
	dst->ownFrames = 0;

	if(src->regSize > 0) {
		/* create array */
		dst->regions = cache_calloc(src->regSize,sizeof(sVMRegion*));
		if(dst->regions == NULL)
			goto error;

		dst->regSize = src->regSize;
		for(i = 0; i < src->regSize; i++) {
			sVMRegion *vm = REG(src,i);
			/* just clone the stack-region of the current thread */
			if(vm && (!(vm->reg->flags & RF_STACK) || thread_hasStackRegion(t,i))) {
				ssize_t res;
				sVMRegion *nvm = vmm_alloc();
				size_t pageCount,swapped;
				if(nvm == NULL)
					goto error;
				mutex_aquire(&vm->reg->lock);
				/* better don't share the file; they may have to read in parallel */
				nvm->binFile = -1;
				nvm->virt = vm->virt;
				nvm->proc = dst;
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
				REG(dst,i) = nvm;

				/* now copy the pages */
				pageCount = BYTES_2_PAGES(nvm->reg->byteCount);
				res = paging_clonePages(&src->pagedir,&dst->pagedir,vm->virt,nvm->virt,pageCount,
						vm->reg->flags & RF_SHAREABLE);
				if(res < 0) {
					mutex_release(&vm->reg->lock);
					goto error;
				}
				dst->ownFrames += res;

				/* update stats */
				if(vm->reg->flags & RF_SHAREABLE) {
					dst->sharedFrames += reg_pageCount(nvm->reg,&swapped);
					dst->swapped += swapped;
				}
				/* add frames to copy-on-write, if not shared */
				else {
					uintptr_t virt = nvm->virt;
					for(j = 0; j < pageCount; j++) {
						if(vm->reg->pageFlags[j] & PF_SWAPPED)
							dst->swapped++;
						/* not when demand-load or swapping is outstanding since we've not loaded it
						 * from disk yet */
						else if(!(vm->reg->pageFlags[j] & (PF_DEMANDLOAD | PF_SWAPPED))) {
							frameno_t frameNo = paging_getFrameNo(&src->pagedir,virt);
							/* if not already done, mark as cow for parent */
							if(!(vm->reg->pageFlags[j] & PF_COPYONWRITE)) {
								vm->reg->pageFlags[j] |= PF_COPYONWRITE;
								if(!cow_add(src->pid,frameNo)) {
									mutex_release(&vm->reg->lock);
									goto error;
								}
								src->sharedFrames++;
								src->ownFrames--;
							}
							/* do it always for the child */
							nvm->reg->pageFlags[j] |= PF_COPYONWRITE;
							if(!cow_add(dst->pid,frameNo)) {
								mutex_release(&vm->reg->lock);
								goto error;
							}
							dst->sharedFrames++;
						}
						virt += PAGE_SIZE;
					}
				}
				mutex_release(&vm->reg->lock);
			}
		}
	}
	proc_release(src,PLOCK_REGIONS);
	proc_release(dst,PLOCK_REGIONS);
	return 0;

error:
	proc_release(src,PLOCK_REGIONS);
	proc_release(dst,PLOCK_REGIONS);
	/* Note that vmm_remove() will undo the cow just for the child; but thats ok since
	 * the parent will get its frame back as soon as it writes to it */
	/* Note also that we don't restore the old frame-counts; for dst it makes no sense because
	 * we'll destroy the process anyway. for src we can't do it because the cow-entries still
	 * exists and therefore its correct. */
	proc_removeRegions(dstId,true);
	/* no need to free regs, since vmm_remove has already done it */
	return -ENOMEM;
}

int vmm_growStackTo(pid_t pid,vmreg_t reg,uintptr_t addr) {
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	sVMRegion *vm = REG(p,reg);
	ssize_t newPages = 0;
	int res = -ENOMEM;
	addr &= ~(PAGE_SIZE - 1);
	/* report failure if its outside (upper / lower) of the region */
	/* note that we assume here that if a thread has multiple stack-regions, they grow towards
	 * each other */
	if(vm->reg->flags & RF_GROWS_DOWN) {
		if(addr < vm->virt + ROUNDUP(vm->reg->byteCount) && addr < vm->virt) {
			res = 0;
			newPages = (vm->virt - addr) / PAGE_SIZE;
		}
	}
	else {
		if(addr >= vm->virt && addr >= vm->virt + ROUNDUP(vm->reg->byteCount)) {
			res = 0;
			newPages = ROUNDUP(addr - (vm->virt + ROUNDUP(vm->reg->byteCount) - 1)) / PAGE_SIZE;
		}
	}

	if(res == 0) {
		/* if its too much, try the next one; if there is none that fits, report failure */
		if(BYTES_2_PAGES(vm->reg->byteCount) + newPages >= MAX_STACK_PAGES - 1)
			res = -ENOMEM;
		/* new pages necessary? */
		else if(newPages > 0) {
			sThread *t = thread_getRunning();
			if(!thread_reserveFrames(t,newPages)) {
				proc_release(p,PLOCK_REGIONS);
				return -ENOMEM;
			}
			res = vmm_doGrow(p,reg,newPages) != 0 ? 0 : -ENOMEM;
			thread_discardFrames(t);
		}
	}
	proc_release(p,PLOCK_REGIONS);
	return res;
}

size_t vmm_grow(pid_t pid,vmreg_t reg,ssize_t amount) {
	size_t res;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(!p)
		return -ESRCH;
	res = vmm_doGrow(p,reg,amount);
	proc_release(p,PLOCK_REGIONS);
	return res;
}

static size_t vmm_doGrow(sProc *p,vmreg_t reg,ssize_t amount) {
	size_t oldSize;
	ssize_t res = -ENOMEM;
	uintptr_t oldVirt,virt;
	sVMRegion *vm;
	vm = REG(p,reg);
	mutex_aquire(&vm->reg->lock);
	assert(reg >= 0 && reg < (vmreg_t)p->regSize && vm != NULL);
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
				uintptr_t end = oldVirt + ROUNDUP(vm->reg->byteCount);
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
				virt = vm->virt + ROUNDUP(oldSize);
			pts = paging_mapTo(&p->pagedir,virt,NULL,amount,mapFlags);
			if(pts < 0) {
				if(vm->reg->flags & RF_GROWS_DOWN)
					vm->virt += amount * PAGE_SIZE;
				mutex_release(&vm->reg->lock);
				return 0;
			}
			p->ownFrames += pts + amount;
		}
		else {
			if(vm->reg->flags & RF_GROWS_DOWN) {
				virt = vm->virt;
				vm->virt += -amount * PAGE_SIZE;
			}
			else
				virt = vm->virt + ROUNDUP(vm->reg->byteCount);
			pts = paging_unmapFrom(&p->pagedir,virt,-amount,true);
			p->ownFrames -= pts - amount;
			p->swapped -= res;
		}
	}

	res = (vm->reg->flags & RF_GROWS_DOWN) ? oldVirt : oldVirt + ROUNDUP(oldSize);
	mutex_release(&vm->reg->lock);
	return res;
}

void vmm_sprintfRegions(sStringBuffer *buf,pid_t pid) {
	size_t i,c = 0;
	sVMRegion *reg;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(p) {
		for(i = 0; i < p->regSize; i++) {
			reg = REG(p,i);
			if(reg != NULL) {
				if(c)
					prf_sprintf(buf,"\n");
				mutex_aquire(&reg->reg->lock);
				prf_sprintf(buf,"VMRegion %d (%p .. %p):\n",i,reg->virt,
						reg->virt + reg->reg->byteCount - 1);
				reg_sprintf(buf,reg->reg,reg->virt);
				mutex_release(&reg->reg->lock);
				c++;
			}
		}
		proc_release(p,PLOCK_REGIONS);
	}
}

void vmm_sprintfMaps(sStringBuffer *buf,pid_t pid) {
	sVMRegion *reg;
	size_t i;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(p) {
		for(i = 0; i < p->regSize; i++) {
			reg = REG(p,i);
			if(reg != NULL) {
				const char *name = "";
				mutex_aquire(&reg->reg->lock);
				if(i == RNO_TEXT)
					name = "text";
				else if(i == RNO_DATA)
					name = "data";
				else if(i == RNO_RODATA)
					name = "rodata";
				else if(reg->reg->flags & RF_STACK)
					name = "stack";
				else if(reg->reg->flags & RF_TLS)
					name = "tls";
				else if(reg->reg->flags & RF_NOFREE)
					name = "phys";
				prf_sprintf(buf,"%-10s %p - %p (%5zuK) %c%c%c%c",name,reg->virt,
						reg->virt + reg->reg->byteCount - 1,reg->reg->byteCount / K,
						(reg->reg->flags & RF_WRITABLE) ? 'w' : '-',
						(reg->reg->flags & RF_EXECUTABLE) ? 'x' : '-',
						(reg->reg->flags & RF_GROWABLE) ? 'g' : '-',
						(reg->reg->flags & RF_SHAREABLE) ? 's' : '-');
				mutex_release(&reg->reg->lock);
				prf_sprintf(buf,"\n");
			}
		}
		proc_release(p,PLOCK_REGIONS);
	}
}

void vmm_printShort(pid_t pid) {
	sVMRegion *reg;
	size_t i;
	sProc *p = proc_getByPid(pid);
	if(p) {
		for(i = 0; i < p->regSize; i++) {
			reg = REG(p,i);
			if(reg != NULL) {
				vid_printf("\t\t%p - %p (%5zuK): ",reg->virt,reg->virt + reg->reg->byteCount - 1,
						reg->reg->byteCount / K);
				reg_printFlags(reg->reg);
				vid_printf("\n");
			}
		}
	}
}

void vmm_print(pid_t pid) {
	sProc *p = proc_getByPid(pid);
	if(p) {
		size_t i,c = 0;
		sVMRegion *reg;
		vid_printf("Regions of proc %d (%s)\n",pid,p->command);
		for(i = 0; i < p->regSize; i++) {
			reg = REG(p,i);
			if(reg != NULL) {
				if(c)
					vid_printf("\n");
				vid_printf("VMRegion %d (%p .. %p):\n",i,reg->virt,
						reg->virt + reg->reg->byteCount - 1);
				reg_print(reg->reg,reg->virt);
				c++;
			}
		}
		if(c == 0)
			vid_printf("- no regions -\n");
	}
}

static bool vmm_demandLoad(sProc *p,sVMRegion *vm,uintptr_t addr) {
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
		assert(!(vm->reg->flags & RF_SHAREABLE));
		if(!loadCount) {
			/* can't fail because we've requested the frame previously and the page-table is present */
			assert(paging_map(addr,NULL,1,PG_PRESENT | PG_WRITABLE) == 0);
			p->ownFrames++;
		}
		memclear((void*)(addr + loadCount),zeroCount);
	}
	return res;
}

static bool vmm_loadFromFile(sProc *p,sVMRegion *vm,uintptr_t addr,size_t loadCount) {
	int err;
	pid_t pid = p->pid;
	void *tempBuf;
	sSLNode *n;
	frameno_t frame;
	uint mapFlags;

	if(vm->binFile < 0) {
		vm->binFile = vfs_openFile(pid,VFS_READ,vm->reg->binary.ino,vm->reg->binary.dev);
		if(vm->binFile < 0) {
			err = vm->binFile;
			goto error;
		}
	}

	/* note that we currently ignore that the file might have changed in the meantime */
	if((err = vfs_seek(pid,vm->binFile,vm->reg->binOffset + (addr - vm->virt),SEEK_SET)) < 0)
		goto error;

	/* first read into a temp-buffer because we can't mark the page as present until
	 * its read from disk. and we can't use a temporary mapping when switching
	 * threads. */
	tempBuf = cache_alloc(PAGE_SIZE);
	if(tempBuf == NULL) {
		err = -ENOMEM;
		goto error;
	}
	err = vfs_readFile(pid,vm->binFile,tempBuf,loadCount);
	if(err != (ssize_t)loadCount) {
		if(err >= 0)
			err = -ENOMEM;
		goto errorFree;
	}

	/* copy into frame */
	frame = paging_demandLoad(tempBuf,loadCount,vm->reg->flags);

	/* free resources not needed anymore */
	cache_free(tempBuf);

	/* map into all pagedirs */
	mapFlags = PG_PRESENT;
	if(vm->reg->flags & RF_WRITABLE)
		mapFlags |= PG_WRITABLE;
	if(vm->reg->flags & RF_EXECUTABLE)
		mapFlags |= PG_EXECUTABLE;
	for(n = sll_begin(vm->reg->procs); n != NULL; n = n->next) {
		sProc *mp = (sProc*)n->data;
		/* the region may be mapped to a different virtual address */
		vmreg_t mprno = vmm_getRNoByRegion(mp,vm->reg);
		sVMRegion *mpreg = REG(mp,mprno);
		assert(mprno != -1);
		/* can't fail */
		assert(paging_mapTo(&mp->pagedir,mpreg->virt + (addr - vm->virt),&frame,1,mapFlags) == 0);
		if(vm->reg->flags & RF_SHAREABLE)
			mp->sharedFrames++;
		else
			mp->ownFrames++;
	}
	return true;

errorFree:
	cache_free(tempBuf);
error:
	log_printf("Demandload page @ %p for proc %s: %s (%d)\n",addr,p->command,strerror(-err),err);
	return false;
}

static sRegion *vmm_getLRURegion(void) {
	sVMRegion *vm;
	sRegion *lru = NULL;
	time_t ts = UINT_MAX;
	mutex_aquire(&regLock);
	vm = regions;
	while(vm != NULL) {
		/* the disk-driver is not swappable */
		if(vm->proc->pid != DISK_PID && !(vm->reg->flags & RF_NOFREE) && vm->reg->timestamp < ts) {
			size_t j,count = 0,pages;
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
		vm = vm->next;
	}
	mutex_release(&regLock);
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
	for(n = sll_begin(reg->procs); n != NULL; n = n->next) {
		sProc *mp = (sProc*)n->data;
		/* the region may be mapped to a different virtual address */
		vmreg_t mprno = vmm_getRNoByRegion(mp,reg);
		sVMRegion *mpreg = REG(mp,mprno);
		assert(mprno != -1);
		/* can't fail */
		assert(paging_mapTo(&mp->pagedir,mpreg->virt + offset,NULL,1,0) == 0);
		if(reg->flags & RF_SHAREABLE)
			mp->sharedFrames--;
		else
			mp->ownFrames--;
		mp->swapped++;
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
	for(n = sll_begin(reg->procs); n != NULL; n = n->next) {
		sProc *mp = (sProc*)n->data;
		/* the region may be mapped to a different virtual address */
		vmreg_t mprno = vmm_getRNoByRegion(mp,reg);
		sVMRegion *mpreg = REG(mp,mprno);
		assert(mprno != -1);
		/* can't fail */
		assert(paging_mapTo(&mp->pagedir,mpreg->virt + offset,&frameNo,1,flags) == 0);
		if(reg->flags & RF_SHAREABLE)
			mp->sharedFrames++;
		else
			mp->ownFrames++;
		mp->swapped--;
	}
}

static vmreg_t vmm_getRegionOf(sProc *p,uintptr_t addr) {
	size_t i;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *vm = REG(p,i);
		if(vm && addr >= vm->virt && addr < vm->virt + ROUNDUP(vm->reg->byteCount))
			return i;
	}
	return -1;
}

static vmreg_t vmm_getRNoByRegion(sProc *p,const sRegion *reg) {
	size_t i;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *vm = REG(p,i);
		if(vm && vm->reg == reg)
			return i;
	}
	return -1;
}

static vmreg_t vmm_findRegIndex(sProc *p,bool text) {
	size_t j;
	/* start behind the fix regions (text, rodata, bss and data) */
	vmreg_t i = text ? 0 : RNO_DATA + 1;
	for(j = 0; j < 2; j++) {
		for(; i < (vmreg_t)p->regSize; i++) {
			if(REG(p,i) == NULL)
				return i;
		}
		/* if the first pass failed, increase regions and try again; start with the current i */
		assert(j == 0);
		if(!vmm_extendRegions(p,i))
			break;
	}
	return -1;
}

static uintptr_t vmm_findFreeStack(sProc *p,size_t byteCount,ulong rflags) {
	uintptr_t addr;
	/* leave a gap between the stacks as a guard */
	if(byteCount > (MAX_STACK_PAGES - 1) * PAGE_SIZE)
		return 0;
#if STACK_AREA_GROWS_DOWN
	uintptr_t end = vmm_getFirstUsableAddr(p,true);
	for(addr = STACK_AREA_END; addr > end; addr -= MAX_STACK_PAGES * PAGE_SIZE) {
		if(vmm_isOccupied(p,addr - (MAX_STACK_PAGES - 1) * PAGE_SIZE,addr) == NULL)
			return addr - ROUNDUP(byteCount);
	}
#else
	for(addr = STACK_AREA_BEGIN; addr < STACK_AREA_END; addr += MAX_STACK_PAGES * PAGE_SIZE) {
		if(vmm_isOccupied(p,addr,addr + (MAX_STACK_PAGES - 1) * PAGE_SIZE) == NULL) {
			if(rflags & RF_GROWS_DOWN)
				return addr + (MAX_STACK_PAGES - 1) * PAGE_SIZE - ROUNDUP(byteCount);
			return addr;
		}
	}
#endif
	return 0;
}

static sVMRegion *vmm_isOccupied(sProc *p,uintptr_t start,uintptr_t end) {
	size_t i;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *reg = REG(p,i);
		if(reg != NULL) {
			uintptr_t rstart = reg->virt;
			uintptr_t rend = reg->virt + ROUNDUP(reg->reg->byteCount);
			if(OVERLAPS(rstart,rend,start,end))
				return reg;
		}
	}
	return NULL;
}

static uintptr_t vmm_getFirstUsableAddr(sProc *p,bool textNData) {
	size_t i;
	uintptr_t addr = 0;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *reg = REG(p,i);
		if(reg != NULL && (!textNData || i <= RNO_DATA) && reg->virt + reg->reg->byteCount > addr)
			addr = reg->virt + reg->reg->byteCount;
	}
	return ROUNDUP(addr);
}

static uintptr_t vmm_findFreeAddr(sProc *p,size_t byteCount) {
	uintptr_t addr;
	if(byteCount > FREE_AREA_END - FREE_AREA_BEGIN)
		return 0;
	for(addr = FREE_AREA_BEGIN; addr + byteCount < FREE_AREA_END; ) {
		sVMRegion *reg = vmm_isOccupied(p,addr,addr + byteCount);
		if(reg == NULL)
			return addr;
		/* try again behind this area */
		addr = reg->virt + ROUNDUP(reg->reg->byteCount);
	}
	return 0;
}

static bool vmm_extendRegions(sProc *p,size_t i) {
	size_t count = MAX(4,i * 2);
	sVMRegion **regs = (sVMRegion **)cache_realloc(p->regions,count * sizeof(sVMRegion*));
	if(regs == NULL)
		return false;
	memclear(regs + p->regSize,(count - p->regSize) * sizeof(sVMRegion*));
	p->regions = regs;
	p->regSize = count;
	return true;
}

static sVMRegion *vmm_alloc(void) {
	sVMRegion *reg = cache_alloc(sizeof(sVMRegion));
	if(reg) {
		mutex_aquire(&regLock);
		reg->next = regions;
		regions = reg;
		mutex_release(&regLock);
	}
	return reg;
}

static void vmm_free(sVMRegion *vm,const sProc *p) {
	/* remove from linked list */
	sVMRegion *reg,*prev;
	mutex_aquire(&regLock);
	prev = NULL;
	reg = regions;
	while(reg != NULL) {
		if(reg == vm) {
			if(prev)
				prev->next = reg->next;
			else
				regions = reg->next;
			break;
		}
		prev = reg;
		reg = reg->next;
	}
	mutex_release(&regLock);

	/* close file */
	if(vm->binFile >= 0) {
		vfs_closeFile(p->pid,vm->binFile);
		vm->binFile = -1;
	}
	cache_free(vm);
}

static int vmm_getAttr(sProc *p,uint type,size_t bCount,ulong *pgFlags,ulong *flags,
		uintptr_t *virt,vmreg_t *rno) {
	*rno = 0;
	switch(type) {
		case REG_TEXT:
			*pgFlags = PF_DEMANDLOAD;
			*flags = RF_SHAREABLE | RF_EXECUTABLE;
			*virt = TEXT_BEGIN;
			*rno = RNO_TEXT;
			break;
		case REG_RODATA:
			*pgFlags = PF_DEMANDLOAD;
			*flags = 0;
			*virt = vmm_getFirstUsableAddr(p,true);
			*rno = RNO_RODATA;
			break;
		case REG_DATA:
			*pgFlags = PF_DEMANDLOAD;
			*flags = RF_GROWABLE | RF_WRITABLE;
			*virt = vmm_getFirstUsableAddr(p,true);
			*rno = RNO_DATA;
			break;
		case REG_STACKUP:
		case REG_STACK:
			*pgFlags = 0;
			*flags = RF_GROWABLE | RF_WRITABLE | RF_STACK;
			if(type == REG_STACK)
				*flags |= RF_GROWS_DOWN;
			*virt = vmm_findFreeStack(p,bCount,*flags);
			if(*virt == 0)
				return -ENOMEM;
			break;

		case REG_SHLIBTEXT:
		case REG_SHLIBDATA:
		case REG_DLDATA:
		case REG_TLS:
		case REG_DEVICE:
		case REG_PHYS:
		case REG_SHM:
			*pgFlags = 0;
			if(type == REG_TLS)
				*flags = RF_WRITABLE | RF_TLS;
			else if(type == REG_DEVICE)
				*flags = RF_WRITABLE | RF_NOFREE;
			else if(type == REG_PHYS)
				*flags = RF_WRITABLE;
			else if(type == REG_SHM)
				*flags = RF_SHAREABLE | RF_WRITABLE;
			/* Note that this assumes for the dynamic linker that everything is put one after
			 * another @ INTERP_TEXT_BEGIN (begin of free area, too). But this is always the
			 * case since when doing exec(), the process is empty except stacks. */
			else if(type == REG_SHLIBTEXT) {
				*flags = RF_SHAREABLE | RF_EXECUTABLE;
				*pgFlags = PF_DEMANDLOAD;
			}
			else if(type == REG_DLDATA) {
				/* its growable to give the dynamic linker the chance to use dynamic memory
				 * (at least until he maps the first region behind this one) */
				*flags = RF_WRITABLE | RF_GROWABLE;
				*pgFlags = PF_DEMANDLOAD;
			}
			else if(type == REG_SHLIBDATA) {
				*flags = RF_WRITABLE;
				*pgFlags = PF_DEMANDLOAD;
			}
			*virt = vmm_findFreeAddr(p,bCount);
			if(*virt == 0)
				return -ENOMEM;
			break;
		default:
			assert(false);
			break;
	}
	/* allocate / increase regions, if necessary */
	if(p->regions == NULL || *rno >= (vmreg_t)p->regSize) {
		size_t count = MAX(4,*rno + 1);
		void *nr = cache_realloc(p->regions,count * sizeof(sVMRegion*));
		if(!nr)
			return -ENOMEM;
		memclear((sVMRegion**)nr + p->regSize,(count - p->regSize) * sizeof(sVMRegion*));
		p->regions = nr;
		p->regSize = count;
	}
	/* now a few checks; do them here because now we're sure that p->regions exists etc. */
	switch(type) {
		case REG_TEXT:
			assert(REG(p,RNO_TEXT) == NULL);
			/* fall through */
		case REG_RODATA:
			assert(REG(p,RNO_RODATA) == NULL);
			/* fall through */
		case REG_DATA:
			assert(REG(p,RNO_DATA) == NULL);
			break;

		case REG_SHLIBTEXT:
		case REG_SHLIBDATA:
		case REG_DLDATA:
		case REG_TLS:
		case REG_STACK:
		case REG_STACKUP:
		case REG_DEVICE:
		case REG_PHYS:
		case REG_SHM:
			*rno = vmm_findRegIndex(p,false);
			if(*rno < 0)
				return -ENOMEM;
			break;
	}
	return 0;
}
