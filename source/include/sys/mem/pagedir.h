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

#include <sys/common.h>
#include <sys/mem/physmem.h>

/* flags for map() */
#define PG_PRESENT				1
#define PG_WRITABLE				2
#define PG_EXECUTABLE			4
#define PG_SUPERVISOR			8
/* tells map() that it gets the frame-address and should convert it to a frame-number first */
#define PG_ADDR_TO_FRAME		16
/* make it a global page */
#define PG_GLOBAL				32
/* tells map() to keep the currently set frame-number */
#define PG_KEEPFRM				64

/* for printing the page-directory */
#define PD_PART_ALL				0
#define PD_PART_USER 			1
#define PD_PART_KERNEL			2
#define PD_PART_KHEAP			4
#define PD_PART_PTBLS			8
#define PD_PART_TEMPMAP			16

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
	 * Removes the access that has been provided by the last getAccess() call
	 */
	static void removeAccess();

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
	 * Copies <count> bytes from <src> to <dst> in user-space.
	 * The destination memory might not be writable!
	 *
	 * @param dst the destination address
	 * @param src the source address
	 * @param count the number of bytes to copy
	 */
	static void copyToUser(void *dst,const void *src,size_t count);

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
	static ssize_t mapToCur(uintptr_t virt,const frameno_t *frames,size_t count,uint flags);

	/**
	 * Convenience method for Proc::getCurPageDir()->unmap(...).
	 */
	static size_t unmapFromCur(uintptr_t virt,size_t count,bool freeFrames);

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
	 * @return the number of allocated ptables or a negative error-code if it failed
	 */
	ssize_t clonePages(PageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,bool share);

	/**
	 * Maps <count> pages starting at <virt> to the given frames in this page-directory.
	 * Note that the frame-number will just be set (and thus, <frames> used) when the flag PG_PRESENT
	 * is specified.
	 *
	 * @param virt the virt start-address
	 * @param frames an array with <count> elements which contains the frame-numbers to use.
	 * 	a NULL-value causes the function to request PhysMem::DEF-frames from mm on its own!
	 * @param count the number of pages to map
	 * @param flags some flags for the pages (PG_*)
	 * @return the number of allocated page-tables or a negative error-code if it failed
	 */
	ssize_t map(uintptr_t virt,const frameno_t *frames,size_t count,uint flags);

	/**
	 * Removes <count> pages starting at <virt> from the page-tables in this page-directory.
	 * If you like the function free's the frames.
	 * Note that the function will delete page-tables, too, if they are empty!
	 *
	 * @param virt the virtual start-address
	 * @param count the number of pages to unmap
	 * @param freeFrames whether the frames should be free'd and not just unmapped
	 * @return the number of free'd ptables
	 */
	size_t unmap(uintptr_t virt,size_t count,bool freeFrames);

	/**
	 * Determines the number of page-tables (in the user-area) in this page-directory
	 *
	 * @return the number of present page-tables
	 */
	size_t getPTableCount() const;

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

	/**
	 * Prints the page at given virtual address
	 *
	 * @param os the output-stream
	 * @param virt the virtual address
	 */
	void printPage(OStream &os,uintptr_t virt) const;
};

#ifdef __i386__
#include <sys/arch/i586/mem/pagedir.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/mem/pagedir.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/mem/pagedir.h>
#endif
