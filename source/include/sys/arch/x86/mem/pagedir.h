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

#include <sys/common.h>
#if defined(__i586__)
#	include <sys/arch/i586/mem/layout.h>
#else
#	include <sys/arch/x86_64/mem/layout.h>
#endif
#include <sys/spinlock.h>
#include <sys/lockguard.h>
#include <sys/cpu.h>
#include <string.h>
#include <assert.h>

/* pte fields */
#define PTE_PRESENT				(1UL << 0)
#define PTE_WRITABLE			(1UL << 1)
#define PTE_NOTSUPER			(1UL << 2)
#define PTE_WRITE_THROUGH		(1UL << 3)
#define PTE_CACHE_DISABLE		(1UL << 4)
#define PTE_ACCESSED			(1UL << 5)
#define PTE_DIRTY				(1UL << 6)
#define PTE_LARGE				(1UL << 7)
#define PTE_GLOBAL				(1UL << 8)
#define PTE_EXISTS				(1UL << 9)
#define PTE_FRAMENO(pte)		(((pte) & ~(PAGE_SIZE - 1)) >> PAGE_BITS)
#define PTE_FRAMENO_MASK		(~(PAGE_SIZE - 1))

#define PT_IDX(addr,lvl)		(((addr) >> (PAGE_BITS + PT_BPL * (lvl))) & ((1 << PT_BPL) - 1))

/* determines whether the given address is on the heap */
#define IS_ON_HEAP(addr)		((uintptr_t)(addr) >= KHEAP_START && \
		(uintptr_t)(addr) < KHEAP_START + KHEAP_SIZE)

/* determines whether the given address is in a shared kernel area */
#define IS_SHARED(addr)			((uintptr_t)(addr) >= KERNEL_AREA && \
								(uintptr_t)(addr) < KSTACK_AREA)

class PageDir : public PageDirBase {
	friend class PageDirBase;

public:
	typedef ulong pte_t;

private:
	class PTAllocator : public Allocator {
	public:
		explicit PTAllocator(uintptr_t physStart,size_t count)
			: _phys(physStart), _end(physStart + count * PAGE_SIZE) {
		}

		virtual frameno_t allocPage() {
			return 0;
		}
		virtual frameno_t allocPT() {
			assert(_phys < _end);
			frameno_t frame = _phys / PAGE_SIZE;
			_phys += PAGE_SIZE;
			return frame;
		}
		virtual void freePage(frameno_t) {
		}
		virtual void freePT(frameno_t) {
		}

	private:
		uintptr_t _phys;
		uintptr_t _end;
	};

public:
	explicit PageDir() : PageDirBase(), pagedir(), freeKStack(), lock() {
	}

	static void flushTLB() {
		CPU::setCR3(CPU::getCR3());
	}

	static void flushAddr(uintptr_t addr) {
		asm volatile ("invlpg (%0)" : : "r" (addr));
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

	static uintptr_t mapToTemp(frameno_t frame);
	static void unmapFromTemp();

	static int crtPageTable(pte_t *pte,uint flags,Allocator &alloc);
	static void printPTE(OStream &os,uintptr_t from,uintptr_t to,pte_t page,int level);

	int mapPage(uintptr_t virt,frameno_t frame,uint flags,Allocator &alloc);
	frameno_t unmapPage(uintptr_t virt);
	pte_t *getPTE(uintptr_t virt,uintptr_t *base) const;
	bool gc(uintptr_t virt,pte_t pte,int level,uint bits,Allocator &alloc);
	size_t countEntries(pte_t pte,int level) const;

	uintptr_t pagedir;
	uintptr_t freeKStack;
	SpinLock lock;

	static uintptr_t freeAreaAddr;
	static uint8_t sharedPtbls[][PAGE_SIZE];
};

inline uintptr_t PageDirBase::getPhysAddr() const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->pagedir;
}

inline bool PageDirBase::isInUserSpace(uintptr_t virt,size_t count) {
	return virt + count <= KERNEL_AREA && virt + count >= virt;
}

inline uintptr_t PageDirBase::getAccess(frameno_t frame) {
	if(frame * PAGE_SIZE < DIR_MAP_AREA_SIZE)
		return DIR_MAP_AREA + frame * PAGE_SIZE;
	return PageDir::mapToTemp(frame);
}

inline void PageDirBase::removeAccess(frameno_t frame) {
	if(frame * PAGE_SIZE >= DIR_MAP_AREA_SIZE)
		PageDir::unmapFromTemp();
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

inline size_t PageDirBase::getPageCount() const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->countEntries(pdir->pagedir,PT_LEVELS);
}
