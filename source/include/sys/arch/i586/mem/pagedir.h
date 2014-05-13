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

#pragma once

#include <esc/common.h>
#include <sys/arch/i586/mem/layout.h>
#include <sys/spinlock.h>
#include <sys/lockguard.h>
#include <sys/cpu.h>
#include <string.h>
#include <assert.h>

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr)	((size_t)((uintptr_t)(addr) / PAGE_SIZE / PT_ENTRY_COUNT))

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr)	((size_t)(((uintptr_t)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT))

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount)	(((size_t)(pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

/* determines whether the given address is on the heap */
#define IS_ON_HEAP(addr)		((uintptr_t)(addr) >= KERNEL_HEAP_START && \
		(uintptr_t)(addr) < KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

/* determines whether the given address is in a shared kernel area */
#define IS_SHARED(addr)			((uintptr_t)(addr) >= KERNEL_AREA && \
								(uintptr_t)(addr) < KERNEL_STACK_AREA)

class PageDir : public PageDirBase {
	friend class PageDirBase;

public:
	typedef uint32_t pte_t;
	typedef uint32_t pde_t;

	explicit PageDir() : PageDirBase(), lastChange(), own(), other(), otherUpdate(), freeKStack() {
	}

	/**
	* Assembler routine to exchange the page-directory to the given one
	*
	* @param physAddr the physical address of the page-directory
	*/
	static void exchangePDir(uintptr_t physAddr);

	/**
	* Reserves page-tables for the whole higher-half and inserts them into the page-directory.
	* This should be done ONCE at the beginning as soon as the physical memory management is set up
	*/
	static void mapKernelSpace();
	static void flushTLB() {
		CPU::setCR3(CPU::getCR3());
	}

	/**
	* Maps the given frames (frame-numbers) to a temporary area (writable, super-visor), so that you
	* can access it. Please use unmapFromTemp() as soon as you're finished!
	*
	* @param frames the frame-numbers
	* @param count the number of frames
	* @return the virtual start-address
	*/
	static uintptr_t mapToTemp(const frameno_t *frames,size_t count);

	/**
	* Unmaps the temporary mappings
	*
	* @param count the number of pages
	*/
	static void unmapFromTemp(size_t count);

	/**
	 * @return the physical address of the page-directory
	 */
	uintptr_t getPhysAddr() const {
		return own;
	}

	/**
	* Creates a kernel-stack at an unused address.
	*
	* @return the used address
	*/
	uintptr_t createKernelStack();

	/**
	* Removes the given kernel-stack
	*
	* @param addr the address of the kernel-stack
	*/
	void removeKernelStack(uintptr_t addr);

private:
	static void setWriteProtection(bool enabled) {
		if(enabled)
			CPU::setCR0(CPU::getCR0() | CPU::CR0_WRITE_PROTECT);
		else
			CPU::setCR0(CPU::getCR0() & ~CPU::CR0_WRITE_PROTECT);
	}

	static void flushPageTable(A_UNUSED uintptr_t virt,A_UNUSED uintptr_t ptables) {
		/* it seems to be much faster to flush the complete TLB instead of flushing just the
		 * affected range (and hoping that less TLB-misses occur afterwards) */
		flushTLB();
	}

	static int crtPageTable(pde_t *pd,uintptr_t ptables,uintptr_t virt,uint flags);
	static size_t remEmptyPt(uintptr_t ptables,size_t pti);
	static void printPageTable(OStream &os,uintptr_t ptables,size_t no,pde_t pde);
	static void printPTE(OStream &os,pte_t page);

	uintptr_t getPTables(PageDir *cur) const;
	ssize_t doMap(uintptr_t virt,const frameno_t *frames,size_t count,uint flags);
	size_t doUnmap(uintptr_t virt,size_t count,bool freeFrames);

	uint64_t lastChange;
	uintptr_t own;
	PageDir *other;
	uint64_t otherUpdate;
	uintptr_t freeKStack;

	static SpinLock tmpMapLock;
	/* TODO we could maintain different locks for userspace and kernelspace; since just the kernel is
	 * shared. it would be better to have a global lock for that and a pagedir-lock for the userspace */
	static SpinLock lock;
	static uintptr_t freeAreaAddr;
};

inline uintptr_t PageDirBase::getAccess(frameno_t frame) {
	return PageDir::mapToTemp(&frame,1);
}

inline void PageDirBase::removeAccess() {
	PageDir::unmapFromTemp(1);
}

inline bool PageDirBase::isInUserSpace(uintptr_t virt,size_t count) {
	return virt + count <= KERNEL_AREA && virt + count >= virt;
}

inline void PageDirBase::copyToUser(void *dst,const void *src,size_t count) {
	/* in this case, we know that we can write to the frame, but it may be mapped as readonly */
	PageDir::setWriteProtection(false);
	memcpy(dst,src,count);
	PageDir::setWriteProtection(true);
}

inline void PageDirBase::zeroToUser(void *dst,size_t count) {
	PageDir::setWriteProtection(false);
	memclear(dst,count);
	PageDir::setWriteProtection(true);
}

inline ssize_t PageDirBase::map(uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	LockGuard<SpinLock> g(&PageDir::lock);
	return static_cast<PageDir*>(this)->doMap(virt,frames,count,flags);
}

inline size_t PageDirBase::unmap(uintptr_t virt,size_t count,bool freeFrames) {
	LockGuard<SpinLock> g(&PageDir::lock);
	return static_cast<PageDir*>(this)->doUnmap(virt,count,freeFrames);
}
