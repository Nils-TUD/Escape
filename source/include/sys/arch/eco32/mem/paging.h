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

#pragma once

#include <esc/common.h>
#include <sys/mem/physmemareas.h>
#include <string.h>
#include <assert.h>

/**
 * Virtual memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                text               |     |
 *             +-----------------------------------+     |
 *             |               rodata              |     |
 *     ---     +-----------------------------------+     |
 *             |                data               |     |
 *      |      |                                   |
 *      v      |                ...                |     u
 *     ---     +-----------------------------------+     s
 *      ^      |                                   |     e
 *      |      |        user-stack thread n        |     r
 *             |                                   |     a
 *     ---     +-----------------------------------+     r
 *             |                ...                |     e
 *     ---     +-----------------------------------+     a
 *      ^      |                                   |
 *      |      |        user-stack thread 0        |     |
 *             |                                   |     |
 * 0x60000000: +-----------------------------------+     |
 *             |  dynlinker (always first, if ex)  |     |
 *             |           shared memory           |     |
 *             |         shared libraries          |     |
 *             |          shared lib data          |     |
 *             |       thread local storage        |     |
 *             |        memory mapped stuff        |     |
 *             |        (in arbitrary order)       |     |
 * 0x80000000: +-----------------------------------+   -----    -----
 *             |         mapped page-tables        |     |        |
 * 0x80400000: +-----------------------------------+     |     not shared pts
 *             |      temp mapped page-tables      |     |        |
 * 0x80800000: +-----------------------------------+            -----
 *             |                                   |     k
 *      |      |            kernel-heap            |     e
 *      v      |                                   |     r
 * 0x80C00000: +-----------------------------------+     r      -----
 *             |       VFS global file table       |     e        |
 * 0x81000000: +-----------------------------------+     a
 *             |             VFS nodes             |          dynamically extending regions
 * 0x81400000: +-----------------------------------+     |
 *             |             sll nodes             |     |        |
 * 0x83400000: +-----------------------------------+     |      -----
 *             |           temp map area           |     |
 * 0x84400000: +-----------------------------------+     |      -----
 *             |           kernel stack            |     |     not shared
 * 0x84401000: +-----------------------------------+     |      -----
 *             |                ...                |     |
 * 0xC0000000: +-----------------------------------+   -----
 *             |                                   |     |
 *             |         kernel code+data          |
 *             |                                   |    directly mapped space
 *             +-----------------------------------+
 *             |                ...                |     |
 * 0xFFFFFFFF: +-----------------------------------+   -----
 */

/* beginning of the kernel-code */
#define KERNEL_START			((uintptr_t)0xC0000000)
/* beginning of the kernel-area */
#define KERNEL_AREA				((uintptr_t)0x80000000)
#define DIR_MAPPED_SPACE		((uintptr_t)0xC0000000)

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT			(PAGE_SIZE / 4)

/* the start of the mapped page-tables area */
#define MAPPED_PTS_START		KERNEL_AREA
/* the start of the temporary mapped page-tables area */
#define TMPMAP_PTS_START		(MAPPED_PTS_START + (PT_ENTRY_COUNT * PAGE_SIZE))

/* page-directories in virtual memory */
#define PAGE_DIR_AREA			(MAPPED_PTS_START + PAGE_SIZE * 512)
/* needed for building a new page-dir */
#define PAGE_DIR_TMP_AREA		(TMPMAP_PTS_START + PAGE_SIZE * 513)

/* the start of the kernel-heap */
#define KERNEL_HEAP_START		(TMPMAP_PTS_START + (PT_ENTRY_COUNT * PAGE_SIZE))
/* the size of the kernel-heap (4 MiB) */
#define KERNEL_HEAP_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE)

/* area for global-file-table */
#define GFT_AREA				(KERNEL_HEAP_START + KERNEL_HEAP_SIZE)
#define GFT_AREA_SIZE			(4 * M)
/* area for vfs-nodes */
#define VFSNODE_AREA			(GFT_AREA + GFT_AREA_SIZE)
#define VFSNODE_AREA_SIZE		(4 * M)
/* area for sll-nodes */
#define SLLNODE_AREA			(VFSNODE_AREA + VFSNODE_AREA_SIZE)
#define SLLNODE_AREA_SIZE		(32 * M)

/* for mapping some pages of foreign processes */
#define TEMP_MAP_AREA			(SLLNODE_AREA + SLLNODE_AREA_SIZE)
#define TEMP_MAP_AREA_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE * 4)

/* our kernel-stack */
#define KERNEL_STACK			(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE)

/* the size of the temporary stack we use at the beginning */
#define TMP_STACK_SIZE			PAGE_SIZE

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr)	((size_t)((uintptr_t)(addr) / PAGE_SIZE / PT_ENTRY_COUNT))

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr)	((size_t)(((uintptr_t)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT))

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount)	(((size_t)(pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

/* start-address of the text in dynamic linker */
#define INTERP_TEXT_BEGIN		0x60000000

/* free area for shared memory, tls, shared libraries, ... */
#define FREE_AREA_BEGIN			0x60000000
#define FREE_AREA_END			KERNEL_AREA

/* the stack-area grows downwards from the free area to data, text and so on */
#define STACK_AREA_GROWS_DOWN	1
#define STACK_AREA_END			FREE_AREA_BEGIN

/* determines whether the given address is on the heap */
#define IS_ON_HEAP(addr) ((uintptr_t)(addr) >= KERNEL_HEAP_START && \
		(uintptr_t)(addr) < KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

/* determines whether the given address is in a shared kernel area; in this case it is "shared"
 * if it is accessed over the directly mapped space. */
#define IS_SHARED(addr)			((uintptr_t)(addr) >= KERNEL_START || \
		((uintptr_t)(addr) >= KERNEL_HEAP_START && (uintptr_t)(addr) < KERNEL_STACK))

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
	 * @return the physical address of the page-directory
	 */
	uintptr_t getPhysAddr() const {
		return phys;
	}

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

	size_t remEmptyPt(uintptr_t ptables,size_t pti);
	uintptr_t getPTables() const;

	uintptr_t phys;
	static uintptr_t curPDir asm("curPDir");
	static uintptr_t otherPDir asm("otherPDir");
};

inline void PageDirBase::makeFirst() {
	static_cast<PageDir*>(this)->phys = PageDir::curPDir;
}

inline uintptr_t PageDirBase::makeAccessible(uintptr_t phys,size_t pages) {
	assert(phys == 0);
	return DIR_MAPPED_SPACE | (PhysMemAreas::alloc(pages) * PAGE_SIZE);
}

inline bool PageDirBase::isInUserSpace(uintptr_t virt,size_t count) {
	return virt + count <= KERNEL_AREA && virt + count >= virt;
}

inline uintptr_t PageDirBase::getAccess(frameno_t frame) {
	return frame * PAGE_SIZE | DIR_MAPPED_SPACE;
}

inline void PageDirBase::removeAccess() {
	/* nothing to do */
}

inline void PageDirBase::copyToFrame(frameno_t frame,const void *src) {
	memcpy((void*)(frame * PAGE_SIZE | DIR_MAPPED_SPACE),src,PAGE_SIZE);
}

inline void PageDirBase::copyFromFrame(frameno_t frame,void *dst) {
	memcpy(dst,(void*)(frame * PAGE_SIZE | DIR_MAPPED_SPACE),PAGE_SIZE);
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
