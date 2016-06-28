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

#include <mem/physmem.h>
#include <mem/layout.h>
#include <assert.h>
#include <common.h>
#include <cppsupport.h>

class PageTables {
public:
	/**
	 * Base class for all allocators. Allocators are responsible for allocating and freeing pages
	 * and page-tables.
	 */
	class Allocator : public CacheAllocatable {
	public:
		explicit Allocator() : _pts(0) {
		}
		virtual ~Allocator() {
		}

		/**
		 * @return the number of allocated and free'd page-tables
		 */
		int pageTables() const {
			return _pts;
		}

		/**
		 * Allocates a frame for a page.
		 */
		virtual frameno_t allocPage() = 0;

		/**
		 * Allocates a frame for a page-table
		 */
		virtual frameno_t allocPT() {
			_pts++;
			return PhysMem::allocate(PhysMem::KERN);
		}

		/**
		 * Frees the given frame that belonged to a page.
		 */
		virtual void freePage(frameno_t frame) = 0;

		/**
		 * Frees the given frame that belonged to a page-table
		 */
		virtual void freePT(frameno_t frame) {
			_pts++;
			PhysMem::free(frame,PhysMem::KERN);
		}

	private:
		int _pts;
	};

	/**
	 * Noop-allocator. Does no allocation and free of pages (but page-tables). For allocPage(), it
	 * returns 0, so that the currently set frame is kept.
	 */
	class NoAllocator : public Allocator {
	public:
		virtual frameno_t allocPage() {
			return 0;
		}
		virtual void freePage(frameno_t) {
		}
	};

	/**
	 * The range allocator returns consecutive frame-numbers, starting at the given one.
	 */
	class RangeAllocator : public Allocator {
	public:
		explicit RangeAllocator(frameno_t frame) : Allocator(), _frame(frame) {
		}

		virtual frameno_t allocPage() {
			return _frame++;
		}
		virtual void freePage(frameno_t);

	private:
		frameno_t _frame;
	};

	/**
	 * The user allocator takes frames from the current thread (which have to be put there
	 * beforehand). It frees them as PhysMem::USR.
	 */
	class UAllocator : public Allocator {
	public:
		explicit UAllocator();

		virtual frameno_t allocPage();
		virtual void freePage(frameno_t frame) {
			PhysMem::free(frame,PhysMem::USR);
		}

	private:
		Thread *_thread;
	};

	/**
	 * The kernel allocator allocates and frees critical frames and expects that no page-tables are
	 * created or freed.
	 */
	class KAllocator : public Allocator {
	public:
		virtual frameno_t allocPage() {
			return PhysMem::allocate(PhysMem::CRIT);
		}
		virtual frameno_t allocPT();
		virtual void freePage(frameno_t frame) {
			PhysMem::free(frame,PhysMem::CRIT);
		}
		virtual void freePT(frameno_t);
	};

	/**
	 * The kernel-stack allocator allocates and frees kernel frames and will also create/free page-
	 * tables.
	 */
	class KStackAllocator : public Allocator {
	public:
		virtual frameno_t allocPage() {
			return PhysMem::allocate(PhysMem::KERN);
		}
		virtual void freePage(frameno_t frame) {
			PhysMem::free(frame,PhysMem::KERN);
		}
	};

	/**
	 * Enables the non-executable bit
	 */
	static void enableNXE() {
		hasNXE = true;
	}

	/**
	 * Removes the entry for <addr> from the TLB.
	 */
	static void flushAddr(uintptr_t addr,bool wasPresent);

	/**
	 * @param addr the virtual address
	 * @param lvl the level for which you want to have the index
	 * @return the page table index for the given address in the given level
	 */
	static inline ulong index(uintptr_t addr,int lvl) {
		return (addr >> (PAGE_BITS + PT_BPL * lvl)) & ((1 << PT_BPL) - 1);
	}

	explicit PageTables() : root() {
	}

	/**
	 * @return the root of the page-table hierarchy
	 */
	pte_t getRoot() const {
		return root;
	}
	/**
	 * Sets the root of the page-table hierarchy.
	 *
	 * @param r the root
	 */
	void setRoot(pte_t r) {
		root = r;
	}

	/**
	 * Determines whether the given page is present
	 *
	 * @param virt the virtual address
	 * @return true if so
	 */
	bool isPresent(uintptr_t virt) const {
		uintptr_t base;
		pte_t *pte = getPTE(virt,&base);
		return pte ? *pte & PTE_PRESENT : false;
	}

	/**
	 * Returns the frame-number of the given virtual address. Assumes that its present.
	 *
	 * @param virt the virtual address
	 * @return the frame-number of the given virtual address
	 */
	frameno_t getFrameNo(uintptr_t virt) const {
		uintptr_t base = virt;
		pte_t *pte = getPTE(virt,&base);
		assert(pte != NULL);
		return PTE_FRAMENO(*pte) + (virt - base) / PAGE_SIZE;
	}

	/**
	 * Clones <count> pages at <virtSrc> to <virtDst> from <this> into <dst>. That means
	 * the flags and frames are copied. Additionally, if <share> is false all present pages will
	 * be marked as not-writable and share the frame in the cloned pagedir so that they can be
	 * copied on write-access. Note that either <this> or <dst> has to be the current page-dir!
	 *
	 * @param dst the destination-pagedir
	 * @param virtSrc the virtual source address
	 * @param virtDst the virtual destination address
	 * @param count the number of pages to copy
	 * @param share whether to share the frames
	 * @return the number of allocated page-tables on success
	 */
	int clone(PageTables *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,bool share);

	/**
	 * Maps <count> pages starting at <virt> in this page-directory.
	 *
	 * @param virt the virt start-address
	 * @param count the number of pages to map
	 * @param alloc the allocator to use for allocating pages/page-tables
	 * @param flags some flags for the pages (PG_*)
	 * @return 1 or 0 on success. 1 means that a TLB shootdown is necessary
	 */
	int map(uintptr_t virt,size_t count,Allocator &alloc,uint flags);

	/**
	 * Removes <count> pages starting at <virt> from the page-tables in this page-directory.
	 *
	 * @param virt the virtual start-address
	 * @param count the number of pages to unmap
	 * @param alloc the allocator to use for freeing pages/page-tables
	 * @return 1 if a TLB shootdown is necessary
	 */
	int unmap(uintptr_t virt,size_t count,Allocator &alloc);

	/**
	 * Counts the number of pages that are currently present in this page-directory
	 *
	 * @return the number of pages
	 */
	size_t getPageCount() const {
		return countEntries(root,PT_LEVELS);
	}

	/**
	 * Prints the given parts from this page-directory
	 *
	 * @param os the output-stream
	 * @param parts the parts to print
	 */
	void print(OStream &os,uint parts) const;

private:
	static int crtPageTable(pte_t *pte,uint flags,Allocator &alloc);
	static void printPTE(OStream &os,uintptr_t from,uintptr_t to,pte_t page,int level);

	int mapPage(uintptr_t virt,frameno_t frame,pte_t flags,Allocator &alloc);
	frameno_t unmapPage(uintptr_t virt);
	pte_t *getPTE(uintptr_t virt,uintptr_t *base) const;
	bool gc(uintptr_t virt,pte_t pte,int level,uint bits,Allocator &alloc);
	size_t countEntries(pte_t pte,int level) const;

	pte_t root;
	static bool hasNXE;
};
