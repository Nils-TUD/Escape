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
#include <sys/mem/swap.h>
#include <sys/task/event.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/real.h>
#include <sys/klock.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/log.h>
#include <errors.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

/**
 * The vmm-module manages the user-part of a process's virtual addressspace. That means it
 * manages the regions that are used by the process (as an instance of sVMRegion) and decides
 * where the regions are placed. So it binds a region to a virtual address via sVMRegion.
 */

#define REG(p,i)			(((sVMRegion**)(p)->regions)[(i)])
#define ROUNDUP(bytes)		(((bytes) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

static vmreg_t vmm_doGetRegionOf(sProc *p,uintptr_t addr);
static bool vmm_doPagefault(uintptr_t addr,sProc *p,sVMRegion *vm);
static void vmm_doRemove(sProc *p,vmreg_t reg);
static bool vmm_demandLoad(sProc *p,sVMRegion *vm,ulong *flags,uintptr_t addr);
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

void vmm_init(void) {
	/* nothing to do */
}

uintptr_t vmm_addPhys(pid_t pid,uintptr_t *phys,size_t bCount,size_t align) {
	vmreg_t reg;
	sVMRegion *vm;
	sAllocStats stats;
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
	/* TODO we have to check somehow whether we have enough mem for the page-tables */
	stats = paging_mapTo(&p->pagedir,vm->virt,frames,pages,PG_PRESENT | PG_WRITABLE);
	if(*phys) {
		/* the page-tables are ours, the pages may be used by others, too */
		p->ownFrames += stats.ptables;
		p->sharedFrames += pages;
	}
	else {
		/* its our own mem; store physical address for the caller */
		p->ownFrames += stats.ptables + stats.frames;
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
		return ERR_INVALID_PID;
	if((res = vmm_getAttr(p,type,bCount,&pgFlags,&flags,&virt,&rno)) < 0)
		goto errProc;
	/* no demand-loading if the binary isn't present */
	if(bin == NULL)
		pgFlags &= ~PF_DEMANDLOAD;

	/* create region */
	res = ERR_NOT_ENOUGH_MEM;
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
	vm->virt = virt;
	REG(p,rno) = vm;

	/* map into process */
	if(type != REG_DEVICE && type != REG_PHYS) {
		sAllocStats stats;
		uint mapFlags = 0;
		size_t pageCount = BYTES_2_PAGES(vm->reg->byteCount);
		if(!(pgFlags & PF_DEMANDLOAD)) {
			mapFlags |= PG_PRESENT;
			if(flags & RF_EXECUTABLE)
				mapFlags |= PG_EXECUTABLE;
		}
		if(flags & RF_WRITABLE)
			mapFlags |= PG_WRITABLE;
		stats = paging_mapTo(&p->pagedir,virt,NULL,pageCount,mapFlags);
		if(flags & RF_SHAREABLE)
			p->sharedFrames += stats.frames;
		else
			p->ownFrames += stats.frames;
		p->ownFrames += stats.ptables;
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

errAdd:
	reg_remFrom(reg,p);
errReg:
	reg_destroy(reg);
errProc:
	proc_release(p,PLOCK_REGIONS);
	return res;
}

int vmm_setRegProt(pid_t pid,vmreg_t rno,ulong flags) {
	size_t i,pgcount;
	sSLNode *n;
	sVMRegion *vmreg;
	sProc *p;
	int res = ERR_SETPROT_IMPOSSIBLE;
	if(!(flags & RF_WRITABLE) && flags != 0)
		return ERR_INVALID_ARGS;
	p = proc_request(pid,PLOCK_REGIONS);
	if(!p)
		return ERR_INVALID_PID;
	vmreg = REG(p,rno);
	klock_aquire(&vmreg->reg->lock);
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
		vmreg_t mprno = vmm_getRNoByRegion(mp->pid,vmreg->reg);
		sVMRegion *mpreg = REG(mp,mprno);
		assert(mprno != -1);
		for(i = 0; i < pgcount; i++) {
			/* determine flags; we can't always mark it present.. */
			uint mapFlags = PG_KEEPFRM;
			if(!(vmreg->reg->pageFlags[i] & (PF_DEMANDLOAD | PF_LOADINPROGRESS | PF_SWAPPED)))
				mapFlags |= PG_PRESENT;
			if(vmreg->reg->flags & RF_EXECUTABLE)
				mapFlags |= PG_EXECUTABLE;
			if(flags & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			paging_mapTo(&mp->pagedir,mpreg->virt + i * PAGE_SIZE,NULL,1,mapFlags);
		}
	}
	res = 0;
error:
	klock_release(&vmreg->reg->lock);
	proc_release(p,PLOCK_REGIONS);
	return res;
}

void vmm_swapOut(sRegion *reg,size_t index) {
	sSLNode *n;
	uintptr_t offset = index * PAGE_SIZE;
	klock_aquire(&reg->lock);
	reg->pageFlags[index] |= PF_SWAPPED;
	for(n = sll_begin(reg->procs); n != NULL; n = n->next) {
		sProc *mp = (sProc*)n->data;
		/* the region may be mapped to a different virtual address */
		vmreg_t mprno = vmm_getRNoByRegion(mp->pid,reg);
		sVMRegion *mpreg = REG(mp,mprno);
		assert(mprno != -1);
		paging_mapTo(&mp->pagedir,mpreg->virt + offset,NULL,1,0);
		if(reg->flags & RF_SHAREABLE)
			mp->sharedFrames--;
		else
			mp->ownFrames--;
		mp->swapped++;
	}
	klock_release(&reg->lock);
}

void vmm_swapIn(sRegion *reg,size_t index,frameno_t frameNo) {
	sSLNode *n;
	uintptr_t offset = index * PAGE_SIZE;
	uint flags = PG_PRESENT;
	klock_aquire(&reg->lock);
	if(reg->flags & RF_WRITABLE)
		flags |= PG_WRITABLE;
	if(reg->flags & RF_EXECUTABLE)
		flags |= PG_EXECUTABLE;
	reg->pageFlags[index] &= ~PF_SWAPPED;
	for(n = sll_begin(reg->procs); n != NULL; n = n->next) {
		sProc *mp = (sProc*)n->data;
		/* the region may be mapped to a different virtual address */
		vmreg_t mprno = vmm_getRNoByRegion(mp->pid,reg);
		sVMRegion *mpreg = REG(mp,mprno);
		assert(mprno != -1);
		paging_mapTo(&mp->pagedir,mpreg->virt + offset,&frameNo,1,flags);
		if(reg->flags & RF_SHAREABLE)
			mp->sharedFrames++;
		else
			mp->ownFrames++;
		mp->swapped--;
	}
	klock_release(&reg->lock);
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

/* TODO this function should be removed/changed, because we can't pass out a region */
sRegion *vmm_getLRURegion(pid_t pid) {
	sRegion *lru = NULL;
	time_t ts = UINT_MAX; /* TODO thats not correct */
	size_t i;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(!p)
		return NULL;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *vm = REG(p,i);
		if(vm && !(vm->reg->flags & RF_NOFREE) && vm->reg->timestamp < ts) {
			size_t j,count = 0,pages = BYTES_2_PAGES(vm->reg->byteCount);
			for(j = 0; j < pages; j++) {
				if(!(vm->reg->pageFlags[j] & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD))) {
					count++;
					break;
				}
			}
			if(count > 0) {
				ts = vm->reg->timestamp;
				lru = vm->reg;
			}
		}
	}
	proc_release(p,PLOCK_REGIONS);
	return lru;
}

size_t vmm_getPgIdxForSwap(const sRegion *reg) {
	size_t i,pages = BYTES_2_PAGES(reg->byteCount);
	size_t index,count = 0;
	for(i = 0; i < pages; i++) {
		if(!(reg->pageFlags[i] & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD)))
			count++;
	}
	index = util_rand() % count;
	for(i = 0; i < pages; i++) {
		if(!(reg->pageFlags[i] & (PF_SWAPPED | PF_COPYONWRITE | PF_DEMANDLOAD))) {
			if(index-- == 0)
				return i;
		}
	}
	/* should never happen */
	assert(false);
	return 0;
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

vmreg_t vmm_getRegionOf(pid_t pid,uintptr_t addr) {
	vmreg_t res = -1;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(p) {
		res = vmm_doGetRegionOf(p,addr);
		proc_release(p,PLOCK_REGIONS);
	}
	return res;
}

static vmreg_t vmm_doGetRegionOf(sProc *p,uintptr_t addr) {
	size_t i;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *vm = REG(p,i);
		if(vm && addr >= vm->virt && addr < vm->virt + ROUNDUP(vm->reg->byteCount))
			return i;
	}
	return -1;
}

vmreg_t vmm_getRNoByRegion(pid_t pid,const sRegion *reg) {
	/* FIXME remove this function */
	size_t i;
	sProc *p = proc_getByPid(pid);
	if(p) {
		for(i = 0; i < p->regSize; i++) {
			sVMRegion *vm = REG(p,i);
			if(vm && vm->reg == reg)
				return i;
		}
	}
	return -1;
}

bool vmm_getRegRange(pid_t pid,vmreg_t reg,uintptr_t *start,uintptr_t *end) {
	bool res = false;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
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

bool vmm_makeCopySafe(sProc *p,USER void *dst,size_t size) {
	ulong *flags;
	sVMRegion *vm;
	uintptr_t addr = (uintptr_t)dst;
	uintptr_t end = addr + size;
	vmreg_t rno;
	/* if its in kernel, its ok */
	if((uintptr_t)dst >= KERNEL_AREA)
		return true;

	rno = vmm_doGetRegionOf(p,addr);
	if(rno < 0)
		return false;
	vm = REG(p,rno);
	while(addr < end) {
		/* out of region? */
		if(addr >= vm->virt + ROUNDUP(vm->reg->byteCount))
			return false;
		/* copy-on-write still enabled? so resolve it */
		flags = vm->reg->pageFlags + (addr - vm->virt) / PAGE_SIZE;
		if(*flags & (PF_DEMANDLOAD | PF_SWAPPED | PF_COPYONWRITE)) {
			if(!vmm_doPagefault(addr,p,vm))
				return false;
		}
		addr += PAGE_SIZE;
	}
	return true;
}

bool vmm_pagefault(uintptr_t addr) {
	sProc *p;
	sVMRegion *vm;
	bool res = false;
	pid_t pid = proc_getRunning();
	vmreg_t rno;

	p = proc_request(pid,PLOCK_REGIONS);
	rno = vmm_doGetRegionOf(p,addr);
	if(rno < 0) {
		proc_release(p,PLOCK_REGIONS);
		return false;
	}
	vm = REG(p,rno);
	res = vmm_doPagefault(addr,p,vm);
	proc_release(p,PLOCK_REGIONS);
	return res;
}

static bool vmm_doPagefault(uintptr_t addr,sProc *p,sVMRegion *vm) {
	bool res = false;
	ulong *flags = vm->reg->pageFlags + (addr - vm->virt) / PAGE_SIZE;
	addr &= ~(PAGE_SIZE - 1);
	if(*flags & PF_DEMANDLOAD)
		res = vmm_demandLoad(p,vm,flags,addr);
	else if(*flags & PF_SWAPPED)
		res = swap_in(p,addr);
	else if(*flags & PF_COPYONWRITE) {
		frameno_t frameNumber = paging_getFrameNo(&p->pagedir,addr);
		size_t frmCount = cow_pagefault(p->pid,addr,frameNumber);
		p->ownFrames += frmCount;
		p->sharedFrames -= frmCount;
		*flags &= ~PF_COPYONWRITE;
		res = true;
	}
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
	size_t i,c = 0;
	sVMRegion *vm = REG(p,reg);
	size_t pcount;
	klock_aquire(&vm->reg->lock);
	pcount = BYTES_2_PAGES(vm->reg->byteCount);
	assert(p && reg < (vmreg_t)p->regSize && vm != NULL);
	assert(reg_remFrom(vm->reg,p));
	if(reg_refCount(vm->reg) == 0) {
		uintptr_t virt = vm->virt;
		/* remove us from cow and unmap the pages (and free frames, if necessary) */
		for(i = 0; i < pcount; i++) {
			sAllocStats stats;
			bool freeFrame = !(vm->reg->flags & RF_NOFREE);
			vassert(!(vm->reg->pageFlags[i] & PF_LOADINPROGRESS),
				"Flags of region @ %p, page %d: %x",vm->virt,i,vm->reg->pageFlags[i]);
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
			stats = paging_unmapFrom(&p->pagedir,virt,1,freeFrame);
			if(vm->reg->flags & RF_SHAREABLE)
				p->sharedFrames -= stats.frames;
			else
				p->ownFrames -= stats.frames;
			p->ownFrames -= stats.ptables;
			virt += PAGE_SIZE;
		}
		/* now destroy region */
		klock_release(&vm->reg->lock);
		reg_destroy(vm->reg);
	}
	else {
		/* no free here, just unmap */
		sAllocStats stats = paging_unmapFrom(&p->pagedir,vm->virt,pcount,false);
		/* in this case its always a shared region because otherwise there wouldn't be other users */
		/* so we have to substract the present content-frames from the shared ones,
		 * and the ptables from ours */
		p->sharedFrames -= reg_presentPageCount(vm->reg);
		p->ownFrames -= stats.ptables;
		klock_release(&vm->reg->lock);
	}
	vmm_free(vm,p);
	REG(p,reg) = NULL;

	/* check whether all regions are NULL */
	for(i = 0; i < p->regSize; i++) {
		vm = REG(p,i);
		if(vm) {
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
	sAllocStats stats;
	size_t pageCount;
	if(srcId == dstId)
		return ERR_INVALID_ARGS;

	src = proc_request(srcId,PLOCK_REGIONS);
	dst = proc_request(dstId,PLOCK_REGIONS);
	if(!src || !dst) {
		if(dst)
			proc_release(dst,PLOCK_REGIONS);
		if(src)
			proc_release(src,PLOCK_REGIONS);
		return ERR_INVALID_PID;
	}

	vm = REG(src,rno);
	klock_aquire(&vm->reg->lock);
	assert(rno >= 0 && rno < (vmreg_t)src->regSize && vm != NULL);
	assert(vm->reg->flags & RF_SHAREABLE);
	nvm = vmm_alloc();
	if(nvm == NULL)
		goto errProc;

	nvm->binFile = -1;
	nvm->reg = vm->reg;
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
	stats = paging_clonePages(&src->pagedir,&dst->pagedir,vm->virt,nvm->virt,pageCount,true);
	klock_release(&vm->reg->lock);
	dst->sharedFrames += stats.frames;
	dst->ownFrames += stats.ptables;
	proc_release(dst,PLOCK_REGIONS);
	proc_release(src,PLOCK_REGIONS);
	return nrno;

errRem:
	reg_remFrom(vm->reg,dst);
errVmm:
	vmm_free(nvm,dst);
errProc:
	klock_release(&vm->reg->lock);
	proc_release(dst,PLOCK_REGIONS);
	proc_release(src,PLOCK_REGIONS);
	return ERR_NOT_ENOUGH_MEM;
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
				sAllocStats stats;
				sVMRegion *nvm = vmm_alloc();
				size_t pageCount;
				if(nvm == NULL)
					goto error;
				klock_aquire(&vm->reg->lock);
				/* better don't share the file; they may have to read in parallel */
				nvm->binFile = -1;
				nvm->virt = vm->virt;
				if(vm->reg->flags & RF_SHAREABLE) {
					reg_addTo(vm->reg,dst);
					nvm->reg = vm->reg;
				}
				else {
					nvm->reg = reg_clone(dst,vm->reg);
					if(nvm->reg == NULL) {
						klock_release(&vm->reg->lock);
						goto error;
					}
				}
				REG(dst,i) = nvm;
				/* now copy the pages */
				pageCount = BYTES_2_PAGES(nvm->reg->byteCount);
				stats = paging_clonePages(&src->pagedir,&dst->pagedir,vm->virt,nvm->virt,pageCount,
						vm->reg->flags & RF_SHAREABLE);
				dst->ownFrames += stats.ptables;
				if(vm->reg->flags & RF_SHAREABLE)
					dst->sharedFrames += stats.frames;
				/* add frames to copy-on-write, if not shared */
				else {
					uintptr_t virt = nvm->virt;
					for(j = 0; j < pageCount; j++) {
						/* remove that flag, because it is possible that a page is currently
						 * demand-loaded. since we've cloned that region, we have to do that again
						 * for ourself, if necessary. */
						nvm->reg->pageFlags[j] &= ~PF_LOADINPROGRESS;
						/* not when demand-load or swapping is outstanding since we've not loaded it
						 * from disk yet */
						if(!(vm->reg->pageFlags[j] & (PF_DEMANDLOAD | PF_SWAPPED))) {
							frameno_t frameNo = paging_getFrameNo(&src->pagedir,virt);
							/* if not already done, mark as cow for parent */
							if(!(vm->reg->pageFlags[j] & PF_COPYONWRITE)) {
								vm->reg->pageFlags[j] |= PF_COPYONWRITE;
								if(!cow_add(src->pid,frameNo)) {
									klock_release(&vm->reg->lock);
									goto error;
								}
								src->sharedFrames++;
								src->ownFrames--;
							}
							/* do it always for the child */
							nvm->reg->pageFlags[j] |= PF_COPYONWRITE;
							if(!cow_add(dst->pid,frameNo)) {
								klock_release(&vm->reg->lock);
								goto error;
							}
							dst->sharedFrames++;
						}
						virt += PAGE_SIZE;
					}
				}
				klock_release(&vm->reg->lock);
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
	return ERR_NOT_ENOUGH_MEM;
}

int vmm_growStackTo(pid_t pid,vmreg_t reg,uintptr_t addr) {
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	sVMRegion *vm = REG(p,reg);
	ssize_t newPages = 0;
	int res = ERR_NOT_ENOUGH_MEM;
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
			res = ERR_NOT_ENOUGH_MEM;
		/* no new pages necessary? then its no failure */
		else if(newPages > 0) {
			proc_release(p,PLOCK_REGIONS);
			return vmm_grow(pid,reg,newPages);
		}
	}
	proc_release(p,PLOCK_REGIONS);
	return res;
}

ssize_t vmm_grow(pid_t pid,vmreg_t reg,ssize_t amount) {
	size_t oldSize;
	ssize_t res = ERR_NOT_ENOUGH_MEM;
	uintptr_t oldVirt,virt;
	sVMRegion *vm;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(!p)
		return ERR_INVALID_PID;

	vm = REG(p,reg);
	klock_aquire(&vm->reg->lock);
	assert(reg >= 0 && reg < (vmreg_t)p->regSize && vm != NULL);
	assert((vm->reg->flags & RF_GROWABLE) && !(vm->reg->flags & RF_SHAREABLE));

	/* check whether we've reached the max stack-pages */
	if((vm->reg->flags & RF_STACK) &&
			BYTES_2_PAGES(vm->reg->byteCount) + amount >= MAX_STACK_PAGES - 1)
		goto error;

	/* swap out, if necessary; FIXME we can't swap here that easily (locks) */
	if(amount > 0 && !swap_outUntil(amount))
		goto error;

	/* check again to be sure */
	vm = REG(p,reg);
	if((vm->reg->flags & RF_STACK) &&
			BYTES_2_PAGES(vm->reg->byteCount) + amount >= MAX_STACK_PAGES - 1)
		goto error;

	/* resize region */
	oldVirt = vm->virt;
	oldSize = vm->reg->byteCount;
	if(amount != 0) {
		sAllocStats stats;
		/* check whether the space is free */
		if(amount > 0) {
			if(vm->reg->flags & RF_GROWS_DOWN) {
				if(vmm_isOccupied(p,oldVirt - amount * PAGE_SIZE,oldVirt))
					goto error;
			}
			else {
				uintptr_t end = oldVirt + ROUNDUP(vm->reg->byteCount);
				if(vmm_isOccupied(p,end,end + amount * PAGE_SIZE))
					goto error;
			}
		}
		if(!reg_grow(vm->reg,amount))
			goto error;

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
			stats = paging_mapTo(&p->pagedir,virt,NULL,amount,mapFlags);
			p->ownFrames += stats.frames + stats.ptables;
		}
		else {
			if(vm->reg->flags & RF_GROWS_DOWN) {
				virt = vm->virt;
				vm->virt += -amount * PAGE_SIZE;
			}
			else
				virt = vm->virt + ROUNDUP(vm->reg->byteCount);
			stats = paging_unmapFrom(&p->pagedir,virt,-amount,true);
			p->ownFrames -= stats.frames + stats.ptables;
		}
	}

	res = ((vm->reg->flags & RF_GROWS_DOWN) ? oldVirt : oldVirt + ROUNDUP(oldSize)) / PAGE_SIZE;
error:
	klock_release(&vm->reg->lock);
	proc_release(p,PLOCK_REGIONS);
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
				klock_aquire(&reg->reg->lock);
				prf_sprintf(buf,"VMRegion %d (%p .. %p):\n",i,reg->virt,
						reg->virt + reg->reg->byteCount - 1);
				reg_sprintf(buf,reg->reg,reg->virt);
				klock_release(&reg->reg->lock);
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
				klock_aquire(&reg->reg->lock);
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
				klock_release(&reg->reg->lock);
				prf_sprintf(buf,"\n");
			}
		}
		proc_release(p,PLOCK_REGIONS);
	}
}

void vmm_printShort(pid_t pid) {
	sVMRegion *reg;
	size_t i;
	sProc *p = proc_request(pid,PLOCK_REGIONS);
	if(p) {
		for(i = 0; i < p->regSize; i++) {
			reg = REG(p,i);
			if(reg != NULL) {
				klock_aquire(&reg->reg->lock);
				vid_printf("\t\t%p - %p (%5zuK): ",reg->virt,reg->virt + reg->reg->byteCount - 1,
						reg->reg->byteCount / K);
				reg_printFlags(reg->reg);
				klock_release(&reg->reg->lock);
				vid_printf("\n");
			}
		}
		proc_release(p,PLOCK_REGIONS);
	}
}

void vmm_print(pid_t pid) {
	sStringBuffer buf;
	buf.dynamic = true;
	buf.len = 0;
	buf.size = 0;
	buf.str = NULL;
	vid_printf("Regions of proc %d (%s)\n",pid,proc_getByPid(pid)->command);
	vmm_sprintfRegions(&buf,pid);
	if(buf.str != NULL)
		vid_printf("%s\n",buf.str);
	else
		vid_printf("- no regions -\n");
	cache_free(buf.str);
}

static bool vmm_demandLoad(sProc *p,sVMRegion *vm,ulong *flags,uintptr_t addr) {
	bool res = false;
	sThread *t = thread_getRunning();

	/* calculate the number of bytes to load and zero */
	size_t loadCount = 0, zeroCount;
	if(addr - vm->virt < vm->reg->loadCount)
		loadCount = MIN(PAGE_SIZE,vm->reg->loadCount - (addr - vm->virt));
	else
		res = true;
	zeroCount = MIN(PAGE_SIZE,vm->reg->byteCount - (addr - vm->virt)) - loadCount;

	klock_aquire(&vm->reg->lock);
	/* check if we should still demand-load this page (may have changed in the meantime) */
	if(!(*flags & PF_DEMANDLOAD)) {
		klock_release(&vm->reg->lock);
		return true;
	}

	/* if another thread already loads it, wait here until he's done */
	if(*flags & PF_LOADINPROGRESS) {
		do {
			ev_wait(t,EVI_VMM_DONE,(evobj_t)vm->reg);
			klock_release(&vm->reg->lock);
			proc_release(p,PLOCK_REGIONS);

			thread_switchNoSigs();

			p = proc_request(p->pid,PLOCK_REGIONS);
			klock_aquire(&vm->reg->lock);
		}
		while(*flags & PF_LOADINPROGRESS);
		res = (*flags & PF_DEMANDLOAD) == 0;
		klock_release(&vm->reg->lock);
		return res;
	}

	*flags |= PF_LOADINPROGRESS;
	klock_release(&vm->reg->lock);

	/* load from file */
	if(loadCount)
		res = vmm_loadFromFile(p,vm,addr,loadCount);

	/* zero the rest, if necessary */
	klock_aquire(&vm->reg->lock);
	if(res && zeroCount) {
		assert(!(vm->reg->flags & RF_SHAREABLE));
		if(!loadCount) {
			sAllocStats stats = paging_map(addr,NULL,1,PG_PRESENT | PG_WRITABLE);
			p->ownFrames += stats.frames;
		}
		memclear((void*)(addr + loadCount),zeroCount);
	}

	/* wakeup all waiting processes */
	*flags &= ~(PF_LOADINPROGRESS | PF_DEMANDLOAD);
	ev_wakeup(EVI_VMM_DONE,(evobj_t)vm->reg);
	klock_release(&vm->reg->lock);
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
		err = ERR_NOT_ENOUGH_MEM;
		goto error;
	}
	/* unlock during context-switch */
	proc_release(p,PLOCK_REGIONS);
	err = vfs_readFile(pid,vm->binFile,tempBuf,loadCount);
	p = proc_request(p->pid,PLOCK_REGIONS);
	if(err != (ssize_t)loadCount) {
		if(err >= 0)
			err = ERR_NOT_ENOUGH_MEM;
		goto errorFree;
	}

	/* ensure that a frame is available; note that its easy here since no one else can demand load
	 * this page while we do it */
	if(!swap_outUntil(1)) {
		err = ERR_NOT_ENOUGH_MEM;
		goto errorFree;
	}
	/* after swapping: page not present anymore or demand-load off? there must be something wrong */
	if(((addr - vm->virt) / PAGE_SIZE) >= BYTES_2_PAGES(vm->reg->byteCount) ||
			!(vm->reg->pageFlags[(addr - vm->virt) / PAGE_SIZE] & PF_DEMANDLOAD)) {
		goto errorFree;
	}

	/* copy into frame */
	klock_aquire(&vm->reg->lock);
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
		vmreg_t mprno = vmm_getRNoByRegion(mp->pid,vm->reg);
		sVMRegion *mpreg = REG(mp,mprno);
		assert(mprno != -1);
		paging_mapTo(&mp->pagedir,mpreg->virt + (addr - vm->virt),&frame,1,mapFlags);
		if(vm->reg->flags & RF_SHAREABLE)
			mp->sharedFrames++;
		else
			mp->ownFrames++;
	}
	klock_release(&vm->reg->lock);
	return true;

errorFree:
	cache_free(tempBuf);
error:
	log_printf("Demandload page @ %p for proc %s: %s (%d)\n",addr,p->command,strerror(err),err);
	return false;
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
	UNUSED(rflags);
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
	return cache_alloc(sizeof(sVMRegion));
}

static void vmm_free(sVMRegion *vm,const sProc *p) {
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
				return ERR_NOT_ENOUGH_MEM;
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
				return ERR_NOT_ENOUGH_MEM;
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
			return ERR_NOT_ENOUGH_MEM;
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
				return ERR_NOT_ENOUGH_MEM;
			break;
	}
	return 0;
}
