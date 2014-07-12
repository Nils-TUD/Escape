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
#include <mem/physmem.h>
#include <mem/pagetables.h>
#include <cppsupport.h>

/* flags for map() */
#define PG_PRESENT				1
#define PG_WRITABLE				2
#define PG_EXECUTABLE			4
#define PG_SUPERVISOR			8
/* make it a global page */
#define PG_GLOBAL				16
#define PG_NOPAGES				32

/* for printing the page-directory */
#define PD_PART_USER 			1
#define PD_PART_KERNEL			2
#define PD_PART_KHEAP			4
#define PD_PART_PTBLS			8
#define PD_PART_NOTSHARED		16
#define PD_PART_ALL				(PD_PART_USER | PD_PART_KERNEL | PD_PART_PTBLS | PD_PART_NOTSHARED)

class PageDir;
class OStream;

class PageDirBase {
protected:
	explicit PageDirBase() {
	}

public:
	/**
	 * Inits the paging. Sets up the page-dir and page-tables for the kernel and enables paging
	 */
	static void init();

	/**
	 * Ensures that <pages> pages are accessible somewhere. If <phys> is not 0, this physical memory
	 * should be made accessible, otherwise arbitrary memory.
	 *
	 * @param phys the physical memory or 0 for any
	 * @param pages the number of pages
	 * @return the chosen address
	 */
	static uintptr_t makeAccessible(uintptr_t phys,size_t pages);

	/**
	 * Checks whether the given range is in user-space
	 *
	 * @param virt the start-address
	 * @param count the number of bytes
	 * @return true if so
	 */
	static bool isInUserSpace(uintptr_t virt,size_t count);

	/**
	 * Ensures that the content of the given frame can be accessed and returns the address to do so.
	 * Please use removeAccess() to free resources. Note that you can't access two frames
	 * in parallel.
	 *
	 * @param frame the frame-number
	 * @return the address
	 */
	static uintptr_t getAccess(frameno_t frame);

	/**
	 * Removes the access that has been provided by the last getAccess() call.
	 *
	 * @param frame the frame-number
	 */
	static void removeAccess(frameno_t frame);

	/**
	 * Finishes the demand-loading-process by copying <loadCount> bytes from <buffer> into a new
	 * frame and returning the frame-number
	 *
	 * @param buffer the buffer
	 * @param loadCount the number of bytes to copy
	 * @param regFlags the flags of the affected region
	 * @return the frame-number
	 */
	static frameno_t demandLoad(const void *buffer,size_t loadCount,ulong regFlags);

	/**
	 * Copies the memory (size=PAGE_SIZE) from <src> into the given frame.
	 *
	 * @param frame the destination-frame
	 * @param src the source (in the current address-space, readable)
	 */
	static void copyToFrame(frameno_t frame,const void *src);

	/**
	 * Copies the memory (size=PAGE_SIZE) from the given frame to <dst>
	 *
	 * @param frame the source-frame
	 * @param dst the destination (in the current address-space, writable)
	 */
	static void copyFromFrame(frameno_t frame,void *dst);

	/**
	 * Copies <count> zeros to <dst>, which is in user-space.
	 * The destination memory might not be writable!
	 *
	 * @param dst the destination address
	 * @param count the number of bytes to copy
	 */
	static void zeroToUser(void *dst,size_t count);

	/**
	 * Clones the kernel-space of the current page-dir into a new one.
	 *
	 * @param dst the destination page-dir
	 * @param tid the id of the thread to clone
	 * @return 0 on success
	 */
	static int cloneKernelspace(PageDir *dst,tid_t tid);

	/**
	 * Convenience method for Proc::getCurPageDir()->map(...).
	 */
	static int mapToCur(uintptr_t virt,size_t count,PageTables::Allocator &alloc,uint flags);

	/**
	 * Convenience method for Proc::getCurPageDir()->unmap(...).
	 */
	static void unmapFromCur(uintptr_t virt,size_t count,PageTables::Allocator &alloc);

	/**
	 * @return the physical address of the page-directory
	 */
	uintptr_t getPhysAddr() const;

	/**
	 * Makes this the first page-directory
	 */
	void makeFirst();

	/**
	 * Destroys this page-directory (not the current!)
	 */
	void destroy();

	/**
	 * Determines whether the given page is present
	 *
	 * @param virt the virtual address
	 * @return true if so
	 */
	bool isPresent(uintptr_t virt) const;

	/**
	 * Returns the frame-number of the given virtual address. Assumes that its present.
	 *
	 * @param virt the virtual address
	 * @return the frame-number of the given virtual address
	 */
	frameno_t getFrameNo(uintptr_t virt) const;

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
	int clone(PageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,bool share);

	/**
	 * Maps <count> pages starting at <virt> in this page-directory.
	 *
	 * @param virt the virt start-address
	 * @param count the number of pages to map
	 * @param alloc the allocator to use for allocating pages/page-tables
	 * @param flags some flags for the pages (PG_*)
	 * @return 0 on success
	 */
	int map(uintptr_t virt,size_t count,PageTables::Allocator &alloc,uint flags);

	/**
	 * Removes <count> pages starting at <virt> from the page-tables in this page-directory.
	 *
	 * @param virt the virtual start-address
	 * @param count the number of pages to unmap
	 * @param alloc the allocator to use for freeing pages/page-tables
	 */
	void unmap(uintptr_t virt,size_t count,PageTables::Allocator &alloc);

	/**
	 * Counts the number of pages that are currently present in this page-directory
	 *
	 * @return the number of pages
	 */
	size_t getPageCount() const;

	/**
	 * Prints the given parts from this page-directory
	 *
	 * @param os the output-stream
	 * @param parts the parts to print
	 */
	void print(OStream &os,uint parts) const;
};

#if defined(__x86__)
#	include <arch/x86/mem/pagedir.h>
#elif defined(__eco32__)
#	include <arch/eco32/mem/pagedir.h>
#elif defined(__mmix__)
#	include <arch/mmix/mem/pagedir.h>
#endif
