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

#include <common.h>
#include <mem/layout.h>
#include <mem/pagetables.h>
#include <spinlock.h>
#include <lockguard.h>
#include <cpu.h>
#include <string.h>
#include <assert.h>

class PageDir : public PageDirBase {
	friend class PageDirBase;

	class PTAllocator : public PageTables::Allocator {
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
	explicit PageDir() : PageDirBase(), freeKStack(), lock(), pts() {
	}

	PageTables *getPageTables() {
		return &pts;
	}

	static void flushTLB() {
		CPU::setCR3(CPU::getCR3());
	}

	/**
	 * Enables NXE if possible.
	 */
	static void enableNXE();

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

	uintptr_t freeKStack;
	SpinLock lock;
	PageTables pts;

	static uintptr_t freeAreaAddr;
	static uint8_t sharedPtbls[][PAGE_SIZE];
};

inline void PageTables::flushAddr(uintptr_t addr,bool wasPresent) {
	if(wasPresent)
		asm volatile ("invlpg (%0)" : : "r" (addr));
}

inline void PageTables::flushPT(uintptr_t) {
	// nothing to do
}

inline uintptr_t PageDirBase::getPhysAddr() const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->pts.getRoot();
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

inline bool PageDirBase::isPresent(uintptr_t virt) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->pts.isPresent(virt);
}

inline frameno_t PageDirBase::getFrameNo(uintptr_t virt) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->pts.getFrameNo(virt);
}

inline void PageDirBase::zeroToUser(void *dst,size_t count) {
	PageDir::setWriteProtection(false);
	memclear(dst,count);
	PageDir::setWriteProtection(true);
}

inline size_t PageDirBase::getPageCount() const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->pts.getPageCount();
}

inline void PageDirBase::print(OStream &os,uint parts) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	pdir->pts.print(os,parts);
}
