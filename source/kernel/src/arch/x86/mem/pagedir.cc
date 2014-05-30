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
#include <sys/mem/pagedir.h>
#include <sys/task/proc.h>
#include <sys/task/smp.h>
#include <sys/util.h>
#include <assert.h>

extern void *proc0TLPD;
uintptr_t PageDir::freeAreaAddr = KFREE_AREA;

/* Note that we only need a lock for the temp-page here, because everything else is not shared
 * among different modules. First, the only critical state here are the page-tables. They are
 * created at boot for the kernel and in map(), join() or fork() for the user-side. For the
 * user-side different regions might share page-tables, but since they are created only with these
 * 3 operations during which we have acquired the process-lock, it's no problem. Changing PTEs
 * in not-locked-processes is ok anyway.
 * On the kernel-side there is the free area, but this is only used at boot time. The kernel-stacks
 * are created holding the process-lock, so that this works, too. Every other area belongs to
 * exactly one module, which of course takes care of locking itself.
 * In total, there is no need to lock any operation (except temp-page). The reason might be a bit
 * too subtle. TODO should we keep it this way?
 */

int PageDirBase::mapToCur(uintptr_t virt,size_t count,PageTables::Allocator &alloc,uint flags) {
	return Proc::getCurPageDir()->map(virt,count,alloc,flags);
}

void PageDirBase::unmapFromCur(uintptr_t virt,size_t count,PageTables::Allocator &alloc) {
	Proc::getCurPageDir()->unmap(virt,count,alloc);
}

void PageDirBase::makeFirst() {
	PageDir *pdir = static_cast<PageDir*>(this);
	pdir->pts.setRoot((pte_t)&proc0TLPD & ~KERNEL_AREA);
	pdir->freeKStack = KSTACK_AREA;
	pdir->lock = SpinLock();
}

uintptr_t PageDirBase::makeAccessible(uintptr_t phys,size_t pages) {
	PageDir *cur = Proc::getCurPageDir();
	uintptr_t addr = PageDir::freeAreaAddr;
	if(addr + pages * PAGE_SIZE > KFREE_AREA + KFREE_AREA_SIZE)
		Util::panic("Bootstrap area too small");
	if(phys) {
		PageTables::RangeAllocator alloc(phys / PAGE_SIZE);
		cur->map(addr,pages,alloc,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	}
	else {
		PageTables::KAllocator alloc;
		cur->map(addr,pages,alloc,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	}
	PageDir::freeAreaAddr += pages * PAGE_SIZE;
	return addr;
}

uintptr_t PageDir::createKernelStack() {
	uintptr_t addr = freeKStack;
	// take care of overflow
	uintptr_t end = KSTACK_AREA + KSTACK_AREA_SIZE - 1;
	PageTables::KStackAllocator alloc;
	while(addr < end) {
		if(!pts.isPresent(addr)) {
			if(map(addr,1,alloc,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
				addr = 0;
			break;
		}
		addr += PAGE_SIZE;
	}
	if(addr == end)
		addr = 0;
	else
		freeKStack = addr + PAGE_SIZE;
	return addr;
}

void PageDir::removeKernelStack(uintptr_t addr) {
	PageTables::KStackAllocator alloc;
	if(addr < freeKStack)
		freeKStack = addr;
	unmap(addr,1,alloc);
}

uintptr_t PageDir::mapToTemp(frameno_t frame) {
	PageDir *cur = Proc::getCurPageDir();
	cur->lock.down();
	PageTables::RangeAllocator alloc(frame);
	cur->map(TEMP_MAP_PAGE,1,alloc,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	return TEMP_MAP_PAGE;
}

void PageDir::unmapFromTemp() {
	PageDir *cur = Proc::getCurPageDir();
	cur->lock.up();
}

frameno_t PageDirBase::demandLoad(const void *buffer,size_t loadCount,A_UNUSED ulong regFlags) {
	frameno_t frame = Thread::getRunning()->getFrame();
	uintptr_t addr = getAccess(frame);
	memcpy((void*)addr,buffer,loadCount);
	removeAccess(frame);
	return frame;
}

void PageDirBase::copyToFrame(frameno_t frame,const void *src) {
	uintptr_t addr = getAccess(frame);
	memcpy((void*)addr,src,PAGE_SIZE);
	removeAccess(frame);
}

void PageDirBase::copyFromFrame(frameno_t frame,void *dst) {
	uintptr_t addr = getAccess(frame);
	memcpy(dst,(void*)addr,PAGE_SIZE);
	removeAccess(frame);
}

int PageDirBase::clone(PageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,bool share) {
	PageDir *pdir = static_cast<PageDir*>(this);
	int res = pdir->pts.clone(&dst->pts,virtSrc,virtDst,count,share);
	if(res >= 0)
		SMP::flushTLB(pdir);
	return res;
}

int PageDirBase::map(uintptr_t virt,size_t count,PageTables::Allocator &alloc,uint flags) {
	PageDir *pdir = static_cast<PageDir*>(this);
	int res = pdir->pts.map(virt,count,alloc,flags);
	if(res == 1)
		SMP::flushTLB(pdir);
	return res;
}

void PageDirBase::unmap(uintptr_t virt,size_t count,PageTables::Allocator &alloc) {
	PageDir *pdir = static_cast<PageDir*>(this);
	int res = pdir->pts.unmap(virt,count,alloc);
	if(res == 1)
		SMP::flushTLB(pdir);
}
