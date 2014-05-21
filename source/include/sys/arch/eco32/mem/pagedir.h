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
#include <sys/arch/eco32/mem/layout.h>
#include <sys/mem/physmemareas.h>
#include <string.h>
#include <assert.h>

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr)	((size_t)((uintptr_t)(addr) / PAGE_SIZE / PT_ENTRY_COUNT))

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr)	((size_t)(((uintptr_t)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT))

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount)	(((size_t)(pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

/* determines whether the given address is on the heap */
#define IS_ON_HEAP(addr) ((uintptr_t)(addr) >= KHEAP_START && \
		(uintptr_t)(addr) < KHEAP_START + KHEAP_SIZE)

/* determines whether the given address is in a shared kernel area; in this case it is "shared"
 * if it is accessed over the directly mapped space. */
#define IS_SHARED(addr)			((uintptr_t)(addr) >= KERNEL_START || \
		((uintptr_t)(addr) >= KHEAP_START && (uintptr_t)(addr) < KERNEL_STACK))

class PageDir : public PageDirBase {
	friend class PageDirBase;

	/* represents a page-directory-entry */
	struct PDEntry {
		/* the physical address of the page-table */
		uint32_t ptFrameNo		: 20,
		/* unused */
								: 9,
		/* can be used by the OS */
		/* Indicates that this pagetable exists (but must not necessarily be present) */
		exists					: 1,
		/* whether the page is writable */
		writable				: 1,
		/* 1 if the page is present in memory */
		present					: 1;
	};

	/* represents a page-table-entry */
	struct PTEntry {
		/* the physical address of the page */
		uint32_t frameNumber	: 20,
		/* unused */
								: 9,
		/* can be used by the OS */
		/* Indicates that this page exists (but must not necessarily be present) */
		exists				: 1,
		/* whether the page is writable */
		writable			: 1,
		/* 1 if the page is present in memory */
		present				: 1;
	};

public:
	explicit PageDir() : PageDirBase(), phys() {
	}

	/**
	* Sets the entry with index <index> to the given translation
	*/
	static void tlbSet(int index,uint virt,uint phys);

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
	/**
	* Removes the entry for <addr> from the TLB
	*/
	static void tlbRemove(uintptr_t addr) asm("tlb_remove");

	/**
	* Flushes the whole page-table including the page in the mapped page-table-area
	*
	* @param virt a virtual address in the page-table
	* @param ptables the page-table mapping-area
	*/
	static void flushPageTable(uintptr_t virt,uintptr_t ptables);

	static void printPageTable(OStream &os,uintptr_t ptables,size_t no,PDEntry *pde);
	static void printPage(OStream &os,PTEntry *page);

	size_t remEmptyPt(uintptr_t ptables,size_t pti,Allocator &alloc);
	uintptr_t getPTables() const;

	uintptr_t phys;
	static uintptr_t curPDir asm("curPDir");
	static uintptr_t otherPDir asm("otherPDir");
};

inline uintptr_t PageDirBase::getPhysAddr() const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	return pdir->phys;
}

inline void PageDirBase::makeFirst() {
	static_cast<PageDir*>(this)->phys = PageDir::curPDir;
}

inline uintptr_t PageDirBase::makeAccessible(uintptr_t phys,size_t pages) {
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

inline void PageDirBase::copyToFrame(frameno_t frame,const void *src) {
	memcpy((void*)(frame * PAGE_SIZE | DIR_MAP_AREA),src,PAGE_SIZE);
}

inline void PageDirBase::copyFromFrame(frameno_t frame,void *dst) {
	memcpy(dst,(void*)(frame * PAGE_SIZE | DIR_MAP_AREA),PAGE_SIZE);
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
