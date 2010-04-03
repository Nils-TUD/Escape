/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <common.h>
#include <mem/region.h>
#include <mem/paging.h>
#include <mem/vmm.h>
#include <mem/cow.h>
#include <mem/kheap.h>
#include <vfs/vfs.h>
#include <vfs/real.h>
#include <errors.h>
#include <string.h>
#include <video.h>
#include <assert.h>
#include <util.h>

#define FREE_AREA_BEGIN		0xA0000000
#define FREE_AREA_END		KERNEL_AREA_V_ADDR

#define REG(p,i)			(((sVMRegion**)(p)->regions)[(i)])
#define ROUNDUP(bytes)		(((bytes) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

static u8 *vmm_getPageFlag(sVMRegion *reg,u32 addr);
static tVMRegNo vmm_getRegionOf(sProc *p,u32 addr);
static s32 vmm_findRegIndex(sProc *p);
static u32 vmm_findFreeStack(sProc *p,u32 byteCount);
static sVMRegion *vmm_isOccupied(sProc *p,u32 start,u32 end);
static u32 vmm_getFirstUsableAddr(sProc *p,bool textNData);
static u32 vmm_findFreeAddr(sProc *p,u32 byteCount);
static bool vmm_extendRegions(sProc *p,u32 i);
static sVMRegion *vmm_alloc(void);
static void vmm_free(sVMRegion *vm);
static s32 vmm_getAttr(sProc *p,u8 type,u32 bCount,u32 *pgFlags,u32 *flags,u32 *virt,s32 *rno);

static u32 nextFree = 0;
static sVMRegion vmRegions[MAX_REGUSE_COUNT];

void vmm_init(void) {
	/* nothing to do */
}

u32 vmm_addPhys(sProc *p,u32 phys,u32 bCount) {
	tVMRegNo reg;
	sVMRegion *vm;
	u32 frmCount,i,pages = BYTES_2_PAGES(bCount);
	u32 *frames = (u32*)kheap_alloc(sizeof(u32) * pages);
	if(frames == NULL)
		return 0;
	reg = vmm_add(p,NULL,0,bCount,REG_PHYS);
	if(reg < 0) {
		kheap_free(frames);
		return 0;
	}
	vm = REG(p,reg);
	for(i = 0; i < pages; i++) {
		frames[i] = phys / PAGE_SIZE;
		phys += PAGE_SIZE;
	}
	/* TODO we have to check somehow wether we have enough mem for the page-tables */
	frmCount = paging_mapTo(p->pagedir,vm->virt,frames,pages,PG_PRESENT | PG_WRITABLE);
	/* the page-tables count as used frames */
	p->frameCount += frmCount - pages;
	kheap_free(frames);
	return vm->virt;
}

tVMRegNo vmm_add(sProc *p,sBinDesc *bin,u32 binOffset,u32 bCount,u8 type) {
	sRegion *reg;
	sVMRegion *vm;
	tVMRegNo rno;
	s32 res;
	u32 mapFlags;
	u32 virt,pgFlags,flags;
	if((res = vmm_getAttr(p,type,bCount,&pgFlags,&flags,&virt,&rno)) < 0)
		return res;
	/* no demand-loading if the binary isn't present */
	if(bin == NULL)
		pgFlags &= ~PF_DEMANDLOAD;
	reg = reg_create(bin,binOffset,bCount,pgFlags,flags);
	if(!reg)
		return ERR_NOT_ENOUGH_MEM;
	if(!reg_addTo(reg,p->pagedir))
		goto errReg;
	vm = vmm_alloc();
	if(vm == NULL)
		goto errAdd;
	vm->reg = reg;
	vm->virt = virt;
	REG(p,rno) = vm;
	mapFlags = 0;
	if(!(pgFlags & (PF_DEMANDLOAD | PF_DEMANDZERO)))
		mapFlags |= PG_PRESENT;
	if(flags & (RF_WRITABLE))
		mapFlags |= PG_WRITABLE;
	if(type != REG_PHYS) {
		p->frameCount += paging_mapTo(p->pagedir,virt,NULL,
				BYTES_2_PAGES(vm->reg->byteCount),mapFlags);
	}
	return rno;

errAdd:
	reg_remFrom(reg,p->pagedir);
errReg:
	reg_destroy(reg);
	return ERR_NOT_ENOUGH_MEM;
}

void vmm_getMemUsage(sProc *p,u32 *paging,u32 *data) {
	u32 i,maxPtbls = 0,ppaging = 0,pdata = 0;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *vm = REG(p,i);
		if(vm) {
			u32 pages = BYTES_2_PAGES(vm->reg->byteCount);
			pdata += pages;
			maxPtbls += PAGES_TO_PTS(pages);
		}
	}
	if(maxPtbls > 0) {
		u32 j,k,*ptbls = kheap_alloc(maxPtbls * sizeof(u32));
		if(ptbls != NULL) {
			for(i = 0; i < p->regSize; i++) {
				sVMRegion *vm = REG(p,i);
				if(vm) {
					u32 rptbls = PAGES_TO_PTS(BYTES_2_PAGES(vm->reg->byteCount));
					u32 pdi = ADDR_TO_PDINDEX(vm->virt);
					for(j = 0; j < rptbls; j++) {
						bool found = false;
						for(k = 0; k < ppaging; k++) {
							if(ptbls[k] == pdi + j) {
								found = true;
								break;
							}
						}
						if(!found)
							ptbls[ppaging++] = pdi + j;
					}
				}
			}
			kheap_free(ptbls);
		}
	}
	*data = pdata;
	/* + pagedir, page-table for kstack and kstack */
	*paging = ppaging + 3;
}

void vmm_getRegRange(sProc *p,tVMRegNo reg,u32 *start,u32 *end) {
	sVMRegion *vm = REG(p,reg);
	assert(p && reg >= 0 && reg < (s32)p->regSize && vm);
	if(start)
		*start = vm->virt;
	if(end)
		*end = vm->virt + ROUNDUP(vm->reg->byteCount);
}

bool vmm_pagefault(u32 addr) {
	sProc *p = proc_getRunning();
	tVMRegNo rno = vmm_getRegionOf(p,addr);
	sVMRegion *vm;
	u8 *flags;
	if(rno < 0)
		return false;
	vm = REG(p,rno);
	flags = vmm_getPageFlag(vm,addr);
	addr &= ~(PAGE_SIZE - 1);
	if(*flags & PF_DEMANDLOAD) {
		bool res = true;
		sFileInfo info;
		sThread *t = thread_getRunning();
		tFileNo file;
		/* file not present or modified? */
		if(vfsr_stat(t->tid,vm->reg->binary.path,&info) < 0 ||
				info.modifytime != vm->reg->binary.modifytime)
			return false;
		file = vfsr_openFile(t->tid,VFS_READ,vm->reg->binary.path);
		if(file < 0)
			return false;
		if(vfs_seek(t->tid,file,vm->reg->binOffset + (addr - vm->virt),SEEK_SET) >= 0) {
			u8 mapFlags = PG_PRESENT;
			if(vm->reg->flags & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			p->frameCount += paging_map(addr,NULL,1,mapFlags);
			/* TODO actually we may have to read less */
			if(vfs_readFile(t->tid,file,(u8*)addr,PAGE_SIZE) < 0)
				res = false;
		}
		vfs_closeFile(t->tid,file);
		if(res)
			*flags &= ~PF_DEMANDLOAD;
		return res;
	}
	else if(*flags & PF_DEMANDZERO) {
		p->frameCount += paging_map(addr,NULL,1,PG_PRESENT | PG_WRITABLE);
		/* TODO actually we may have to clear less */
		memclear((void*)addr,PAGE_SIZE);
		*flags &= ~PF_DEMANDZERO;
		return true;
	}
	else if(*flags & PF_SWAPPED) {
		/* TODO */
		util_panic("No swapping yet");
	}
	else if(*flags & PF_COPYONWRITE) {
		p->frameCount += cow_pagefault(addr);
		*flags &= ~PF_COPYONWRITE;
		return true;
	}
	return false;
}

void vmm_removeAll(sProc *p,bool remStack) {
	u32 i;
	assert(p);
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *vm = REG(p,i);
		if(vm && (!(vm->reg->flags & RF_STACK) || remStack))
			vmm_remove(p,i);
	}
}

void vmm_remove(sProc *p,tVMRegNo reg) {
	u32 i,c = 0;
	sVMRegion *vm = REG(p,reg);
	assert(p && reg < (s32)p->regSize && vm != NULL);
	assert(reg_remFrom(vm->reg,p->pagedir));
	if(reg_refCount(vm->reg) == 0) {
		u32 pcount = BYTES_2_PAGES(vm->reg->byteCount), virt = vm->virt;
		/* remove us from cow and unmap the pages (and free frames, if necessary) */
		for(i = 0; i < pcount; i++) {
			bool freeFrame = true;
			if(vm->reg->pageFlags[i] & PF_COPYONWRITE) {
				u32 foundOther;
				/* we can free the frame if there is no other user */
				p->frameCount -= cow_remove(p,paging_getFrameNo(virt),&foundOther);
				freeFrame = !foundOther;
			}
			p->frameCount -= paging_unmapFrom(p->pagedir,virt,1,freeFrame);
			virt += PAGE_SIZE;
		}
		/* now destroy region */
		reg_destroy(vm->reg);
	}
	else {
		/* no free here, just unmap */
		p->frameCount -= paging_unmapFrom(p->pagedir,vm->virt,
				BYTES_2_PAGES(vm->reg->byteCount),false);
	}
	REG(p,reg) = NULL;

	/* check wether all regions are NULL */
	for(i = 0; i < p->regSize; i++) {
		vm = REG(p,i);
		if(vm) {
			c++;
			break;
		}
	}
	/* free regions, if all have been removed */
	if(c == 0) {
		kheap_free(p->regions);
		p->regSize = 0;
		p->regions = NULL;
	}
}

tVMRegNo vmm_join(sProc *src,tVMRegNo rno,sProc *dst) {
	sVMRegion *vm = REG(src,rno),*nvm;
	tVMRegNo nrno;
	assert(src && dst && src != dst);
	assert(rno >= 0 && rno < (s32)src->regSize && vm != NULL);
	assert(vm->reg->flags & RF_SHAREABLE);
	nvm = vmm_alloc();
	if(nvm == NULL)
		return ERR_NOT_ENOUGH_MEM;
	nvm->reg = vm->reg;
	nvm->virt = vmm_findFreeAddr(dst,vm->reg->byteCount);
	if(nvm->virt == 0)
		goto errVmm;
	if(!reg_addTo(vm->reg,dst->pagedir))
		goto errVmm;
	nrno = vmm_findRegIndex(dst);
	if(nrno < 0)
		goto errRem;
	REG(dst,nrno) = nvm;
	dst->frameCount += paging_clonePages(src->pagedir,dst->pagedir,vm->virt,nvm->virt,
			BYTES_2_PAGES(vm->reg->byteCount),true);
	return nrno;

errRem:
	reg_remFrom(vm->reg,dst->pagedir);
errVmm:
	vmm_free(nvm);
	return ERR_NOT_ENOUGH_MEM;
}

s32 vmm_cloneAll(sProc *dst) {
	u32 i,j;
	sVMRegion **regs;
	sThread *t = thread_getRunning();
	sProc *src = proc_getRunning();
	assert(dst && dst->regions == NULL && dst->regSize == 0);
	/* create array */
	regs = (sVMRegion **)kheap_calloc(src->regSize,sizeof(sVMRegion*));
	if(regs == NULL)
		return ERR_NOT_ENOUGH_MEM;

	for(i = 0; i < src->regSize; i++) {
		sVMRegion *vm = REG(src,i);
		/* just clone the stack-region of the current thread */
		if(vm && (!(vm->reg->flags & RF_STACK) || (tVMRegNo)i == t->stackRegion)) {
			sVMRegion *nvm = vmm_alloc();
			if(nvm == NULL)
				goto error;
			nvm->virt = vm->virt;
			if(vm->reg->flags & RF_SHAREABLE) {
				reg_addTo(vm->reg,dst->pagedir);
				nvm->reg = vm->reg;
			}
			else {
				nvm->reg = reg_clone(dst->pagedir,vm->reg);
				if(nvm->reg == NULL)
					goto error;
			}
			regs[i] = nvm;
			/* now copy the pages */
			dst->frameCount += paging_clonePages(src->pagedir,dst->pagedir,vm->virt,
					nvm->virt,BYTES_2_PAGES(nvm->reg->byteCount),vm->reg->flags & RF_SHAREABLE);
			/* add frames to copy-on-write, if not shared */
			if(!(vm->reg->flags & RF_SHAREABLE)) {
				u32 pcount = BYTES_2_PAGES(vm->reg->byteCount), virt = nvm->virt;
				for(j = 0; j < pcount; j++) {
					/* not when using demand-loading since we've not loaded it from disk yet */
					if(!(vm->reg->pageFlags[j] & (PF_DEMANDLOAD | PF_DEMANDZERO))) {
						u32 frameNo = paging_getFrameNo(virt);
						/* if not already done, mark as cow for parent */
						if(!(vm->reg->pageFlags[j] & PF_COPYONWRITE)) {
							vm->reg->pageFlags[j] |= PF_COPYONWRITE;
							if(!cow_add(src,frameNo,true))
								goto error;
						}
						/* do it always for the child */
						nvm->reg->pageFlags[j] |= PF_COPYONWRITE;
						if(!cow_add(dst,frameNo,false))
							goto error;
						/* we don't own the frame yet */
						dst->frameCount--;
					}
					virt += PAGE_SIZE;
				}
			}
		}
	}
	dst->regions = regs;
	dst->regSize = src->regSize;
	return 0;

error:
	/* remove the regions that have already been added */
	/* set them first, because vmm_remove() uses them */
	dst->regions = regs;
	dst->regSize = src->regSize;
	/* Note that vmm_remove() will undo the cow just for the child; but thats ok since
	 * the parent will get its frame back as soon as it writes to it */
	vmm_removeAll(dst,true);
	/* no need to free regs, since vmm_remove has already done it */
	return ERR_NOT_ENOUGH_MEM;
}

s32 vmm_growStackTo(sThread *t,u32 addr) {
	sVMRegion *vm = REG(t->proc,t->stackRegion);
	s32 newPages;
	addr &= ~(PAGE_SIZE - 1);
	newPages = (vm->virt - addr) / PAGE_SIZE;
	if(newPages > 0)
		return vmm_grow(t->proc,t->stackRegion,newPages);
	return 0;
}

s32 vmm_grow(sProc *p,tVMRegNo reg,s32 amount) {
	u32 oldSize,oldVirt,virt;
	sVMRegion *vm = REG(p,reg);
	assert(p && reg >= 0 && reg < (s32)p->regSize && vm != NULL);
	assert((vm->reg->flags & RF_GROWABLE) && !(vm->reg->flags & RF_SHAREABLE));

	/* check wether we've reached the max stack-pages */
	if(vm->reg->flags & RF_STACK && BYTES_2_PAGES(vm->reg->byteCount) + amount >= MAX_STACK_PAGES - 1)
		return ERR_NOT_ENOUGH_MEM;

	/* resize region */
	oldVirt = vm->virt;
	oldSize = vm->reg->byteCount;
	if(amount != 0) {
		/* not enough mem? TODO we should use swapping later */
		if(amount > 0 && (s32)mm_getFreeFrmCount(MM_DEF) < amount)
			return ERR_NOT_ENOUGH_MEM;
		if(!reg_grow(vm->reg,amount))
			return ERR_NOT_ENOUGH_MEM;

		/* map / unmap pages */
		if(amount > 0) {
			u32 mapFlags = PG_PRESENT;
			if(vm->reg->flags & RF_WRITABLE)
				mapFlags |= PG_WRITABLE;
			/* stack-pages are added before the existing */
			if(vm->reg->flags & RF_STACK) {
				vm->virt -= amount * PAGE_SIZE;
				virt = vm->virt;
			}
			else
				virt = vm->virt + ROUNDUP(oldSize);
			p->frameCount += paging_mapTo(p->pagedir,virt,NULL,amount,mapFlags);
		}
		else {
			if(vm->reg->flags & RF_STACK) {
				virt = vm->virt;
				vm->virt += -amount * PAGE_SIZE;
			}
			else
				virt = vm->virt + ROUNDUP(vm->reg->byteCount);
			p->frameCount -= paging_unmapFrom(p->pagedir,virt,-amount,true);
		}
	}
	return ((vm->reg->flags & RF_STACK) ? oldVirt : oldVirt + ROUNDUP(oldSize)) / PAGE_SIZE;
}

static u8 *vmm_getPageFlag(sVMRegion *reg,u32 addr) {
	return reg->reg->pageFlags + (addr - reg->virt) / PAGE_SIZE;
}

static tVMRegNo vmm_getRegionOf(sProc *p,u32 addr) {
	u32 i;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *vm = REG(p,i);
		if(vm && addr >= vm->virt && addr < vm->virt + ROUNDUP(vm->reg->byteCount))
			return i;
	}
	return -1;
}

static tVMRegNo vmm_findRegIndex(sProc *p) {
	u32 j;
	/* start behind the fix regions (text, rodata, bss and data) */
	tVMRegNo i = RNO_DATA + 1;
	for(j = 0; j < 2; j++) {
		for(; i < (s32)p->regSize; i++) {
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

static u32 vmm_findFreeStack(sProc *p,u32 byteCount) {
	u32 addr,end;
	if(byteCount > (MAX_STACK_PAGES - 1) * PAGE_SIZE)
		return 0;
	end = vmm_getFirstUsableAddr(p,true);
	for(addr = FREE_AREA_BEGIN; addr > end; addr -= MAX_STACK_PAGES * PAGE_SIZE) {
		/* leave a gap between the stacks as a guard */
		if(vmm_isOccupied(p,addr - (MAX_STACK_PAGES - 1) * PAGE_SIZE,addr) == NULL)
			return addr - ROUNDUP(byteCount);
	}
	return 0;
}

static sVMRegion *vmm_isOccupied(sProc *p,u32 start,u32 end) {
	u32 i;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *reg = REG(p,i);
		if(reg != NULL) {
			if(OVERLAPS(reg->virt,reg->virt + ROUNDUP(reg->reg->byteCount),start,end))
				return reg;
		}
	}
	return NULL;
}

static u32 vmm_getFirstUsableAddr(sProc *p,bool textNData) {
	u32 i;
	u32 addr = 0;
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *reg = REG(p,i);
		if(reg != NULL && (!textNData || i <= RNO_DATA) && reg->virt + reg->reg->byteCount > addr)
			addr = reg->virt + reg->reg->byteCount;
	}
	return ROUNDUP(addr);
}

static u32 vmm_findFreeAddr(sProc *p,u32 byteCount) {
	u32 addr;
	if(byteCount > FREE_AREA_END - FREE_AREA_BEGIN)
		return 0;
	for(addr = FREE_AREA_BEGIN; addr + byteCount < FREE_AREA_END; ) {
		sVMRegion *reg = vmm_isOccupied(p,addr,addr + byteCount);
		if(reg == NULL)
			return addr;
		/* try again behind this area */
		addr = ROUNDUP(reg->reg->byteCount);
	}
	return 0;
}

static bool vmm_extendRegions(sProc *p,u32 i) {
	u32 count = MAX(4,i * 2);
	sVMRegion **regs = (sVMRegion **)kheap_realloc(p->regions,count * sizeof(sVMRegion*));
	if(regs == NULL)
		return false;
	memclear(regs + p->regSize,(count - p->regSize) * sizeof(sVMRegion*));
	p->regions = regs;
	p->regSize = count;
	return true;
}

static sVMRegion *vmm_alloc(void) {
	u32 i;
	for(i = nextFree; i < MAX_REGUSE_COUNT; i++) {
		if(vmRegions[i].reg == NULL) {
			nextFree = i + 1;
			return vmRegions + i;
		}
	}
	for(i = 0; i < nextFree; i++) {
		if(vmRegions[i].reg == NULL) {
			nextFree = i + 1;
			return vmRegions + i;
		}
	}
	return NULL;
}

static void vmm_free(sVMRegion *vm) {
	u32 i = vm - vmRegions;
	vm->reg = NULL;
	if(i < nextFree)
		nextFree = i;
}

static s32 vmm_getAttr(sProc *p,u8 type,u32 bCount,u32 *pgFlags,u32 *flags,u32 *virt,tVMRegNo *rno) {
	*rno = 0;
	switch(type) {
		case REG_TEXT:
			*pgFlags = PF_DEMANDLOAD;
			*flags = RF_SHAREABLE;
			*virt = TEXT_BEGIN;
			*rno = RNO_TEXT;
			break;
		case REG_RODATA:
			*pgFlags = PF_DEMANDLOAD;
			*flags = 0;
			*virt = vmm_getFirstUsableAddr(p,true);
			*rno = RNO_RODATA;
			break;
		case REG_BSS:
			*pgFlags = PF_DEMANDZERO;
			*flags = RF_WRITABLE;
			*virt = vmm_getFirstUsableAddr(p,true);
			*rno = RNO_BSS;
			break;
		case REG_DATA:
			*pgFlags = PF_DEMANDLOAD;
			*flags = RF_GROWABLE | RF_WRITABLE;
			*virt = vmm_getFirstUsableAddr(p,true);
			*rno = RNO_DATA;
			break;
		case REG_STACK:
			*pgFlags = 0;
			*flags = RF_GROWABLE | RF_WRITABLE | RF_STACK;
			*virt = vmm_findFreeStack(p,bCount);
			if(*virt == 0)
				return ERR_NOT_ENOUGH_MEM;
			break;
		case REG_PHYS:
		case REG_SHM:
			*pgFlags = 0;
			if(type == REG_PHYS)
				*flags = RF_WRITABLE;
			else
				*flags = RF_SHAREABLE | RF_WRITABLE;
			*virt = vmm_findFreeAddr(p,bCount);
			if(*virt == 0)
				return ERR_NOT_ENOUGH_MEM;
			break;
		default:
			assert(false);
			break;
	}
	/* allocate / increase regions, if necessary */
	if(p->regions == NULL || *rno >= (tVMRegNo)p->regSize) {
		u32 count = MAX(4,*rno + 1);
		void *nr = kheap_realloc(p->regions,count * sizeof(sVMRegion*));
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
		case REG_BSS:
			assert(REG(p,RNO_BSS) == NULL);
			/* fall through */
		case REG_DATA:
			assert(REG(p,RNO_DATA) == NULL);
			break;

		case REG_STACK:
		case REG_PHYS:
		case REG_SHM:
			*rno = vmm_findRegIndex(p);
			if(*rno < 0)
				return ERR_NOT_ENOUGH_MEM;
			break;
	}
	return 0;
}


#if DEBUGGING

void vmm_dbg_print(sProc *p) {
	u32 i;
	sVMRegion *reg;
	vid_printf("Regions of proc %d (%s)\n",p->pid,p->command);
	for(i = 0; i < p->regSize; i++) {
		reg = REG(p,i);
		if(reg != NULL) {
			vid_printf("\tVMRegion %d (@ %#08x .. %#08x):\n",i,reg->virt,
					reg->virt + ROUNDUP(reg->reg->byteCount) - 1);
			reg_dbg_print(reg->reg);
			vid_printf("\n");
		}
	}
}

#endif
