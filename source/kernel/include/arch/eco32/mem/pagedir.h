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

#include <mem/layout.h>
#include <mem/pagetables.h>
#include <mem/physmemareas.h>
#include <assert.h>
#include <common.h>
#include <string.h>

/* determines whether the given address is on the heap */
#define IS_ON_HEAP(addr) ((uintptr_t)(addr) >= KHEAP_START && \
		(uintptr_t)(addr) < KHEAP_START + KHEAP_SIZE)

class PageDir : public PageDirBase {
	friend class PageDirBase;

public:
	explicit PageDir() : PageDirBase(), pts() {
	}

	PageTables *getPageTables() {
		return &pts;
	}

	/**
	* Sets the entry with index <index> to the given translation
	*/
	static void tlbSet(int index,uint virt,uint phys);
	/**
	* Removes the entry for <addr> from the TLB
	*/
	static void tlbRemove(uintptr_t addr) asm("tlb_remove");

	/**
	* Prints the contents of the TLB
	*
	* @param os the output-stream
	*/
	void printTLB(OStream &os) const;

private:
	/**
	* Fetches the entry at index <index> from the TLB and puts the virtual address into *entryHi and
	* the physical address including flags into *entryLo.
	*/
	static void tlbGet(int index,uint *entryHi,uint *entryLo);

	PageTables pts;
	static uintptr_t curPDir asm("curPDir");
};

inline void PageTables::flushAddr(uintptr_t addr,bool) {
	/* on ECO32, we also need to remove the entry if it was not present */
	PageDir::tlbRemove(addr);
}

inline uintptr_t PageDirBase::getPhysAddr() const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->pts.getRoot();
}

inline void PageDirBase::makeFirst() {
	PageDir *pdir = static_cast<PageDir*>(this);
	pdir->pts.setRoot(PageDir::curPDir & ~DIR_MAP_AREA);
}

inline uintptr_t PageDirBase::makeAccessible(A_UNUSED uintptr_t phys,size_t pages) {
	assert(phys == 0);
	return DIR_MAP_AREA | (PhysMemAreas::alloc(pages) * PAGE_SIZE);
}

inline bool PageDirBase::isInUserSpace(uintptr_t virt,size_t count) {
	return virt + count <= KERNEL_AREA && virt + count >= virt;
}

inline uintptr_t PageDirBase::getAccess(frameno_t frame) {
	return frame * PAGE_SIZE | DIR_MAP_AREA;
}

inline void PageDirBase::removeAccess(A_UNUSED frameno_t frame) {
	/* nothing to do */
}

inline bool PageDirBase::isPresent(uintptr_t virt) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->pts.isPresent(virt);
}

inline frameno_t PageDirBase::getFrameNo(uintptr_t virt) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->pts.getFrameNo(virt);
}

inline void PageDirBase::copyToFrame(frameno_t frame,const void *src) {
	memcpy((void*)(frame * PAGE_SIZE | DIR_MAP_AREA),src,PAGE_SIZE);
}

inline void PageDirBase::copyFromFrame(frameno_t frame,void *dst) {
	memcpy(dst,(void*)(frame * PAGE_SIZE | DIR_MAP_AREA),PAGE_SIZE);
}

inline int PageDirBase::clone(PageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,bool share) {
	PageDir *pdir = static_cast<PageDir*>(this);
	return pdir->pts.clone(&dst->pts,virtSrc,virtDst,count,share);
}

inline int PageDirBase::map(uintptr_t virt,size_t count,PageTables::Allocator &alloc,uint flags) {
	PageDir *pdir = static_cast<PageDir*>(this);
	return pdir->pts.map(virt,count,alloc,flags);
}

inline void PageDirBase::unmap(uintptr_t virt,size_t count,PageTables::Allocator &alloc) {
	PageDir *pdir = static_cast<PageDir*>(this);
	pdir->pts.unmap(virt,count,alloc);
}

inline size_t PageDirBase::getPageCount() const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->pts.getPageCount();
}

inline void PageDirBase::print(OStream &os,uint parts) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	pdir->pts.print(os,parts);
}

inline void PageDir::tlbSet(int index,uint virt,uint phys) {
	asm volatile (
		"mvts	%0,1\n"	// set index
		"mvts	%1,2\n"	// set entryHi
		"mvts	%2,3\n" // set entryLo
		"tbwi\n"		// write TLB entry at index
		:  : "r"(index), "r"(virt), "r"(phys)
	);
}

inline void PageDir::tlbGet(int index,uint *entryHi,uint *entryLo) {
	asm volatile (
		"mvts	%2,1\n"
		"tbri\n"
		"mvfs	%0,2\n"
		"mvfs	%1,3\n"
		: "=r"(*entryHi), "=r"(*entryLo) : "r"(index)
	);
}
