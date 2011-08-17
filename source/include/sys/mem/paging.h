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

#ifndef PAGING_H_
#define PAGING_H_

#include <sys/common.h>
#include <sys/mem/pmem.h>
#include <sys/printf.h>

#ifdef __i386__
#include <sys/arch/i586/mem/paging.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/mem/paging.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/mem/paging.h>
#endif

/* flags for paging_map() */
#define PG_PRESENT				1
#define PG_WRITABLE				2
#define PG_EXECUTABLE			4
#define PG_SUPERVISOR			8
/* tells paging_map() that it gets the frame-address and should convert it to a frame-number first */
#define PG_ADDR_TO_FRAME		16
/* make it a global page */
#define PG_GLOBAL				32
/* tells paging_map() to keep the currently set frame-number */
#define PG_KEEPFRM				64

/* for printing the page-directory */
#define PD_PART_ALL				0
#define PD_PART_USER 			1
#define PD_PART_KERNEL			2
#define PD_PART_KHEAP			4
#define PD_PART_PTBLS			8
#define PD_PART_TEMPMAP			16

typedef struct {
	size_t ptables;
	size_t frames;
} sAllocStats;

/**
 * Inits the paging. Sets up the page-dir and page-tables for the kernel and enables paging
 */
void paging_init(void);

/**
 * Sets the first page-directory into the given one
 *
 * @param pdir the page-directory-container
 */
void paging_setFirst(tPageDir *pdir);

/**
 * Checks whether the given range is in user-space
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isInUserSpace(uintptr_t virt,size_t count);

/**
 * Clones the kernel-space of the current page-dir into a new one.
 *
 * @param pdir will be set to the page-directory address (physical)
 * @return 0 on success
 */
int paging_cloneKernelspace(tPageDir *pdir);

/**
 * Destroys the given page-directory (not the current!)
 *
 * @param pdir the page-dir
 */
void paging_destroyPDir(tPageDir *pdir);

/**
 * Determines whether the given page is present
 *
 * @param pdir the page-dir
 * @param virt the virtual address
 * @return true if so
 */
bool paging_isPresent(tPageDir *pdir,uintptr_t virt);

/**
 * Returns the frame-number of the given virtual address in the given pagedir. Assumes that
 * its present.
 *
 * @param pdir the page-dir
 * @param virt the virtual address
 * @return the frame-number of the given virtual address
 */
frameno_t paging_getFrameNo(tPageDir *pdir,uintptr_t virt);

/**
 * Finishes the demand-loading-process by copying <loadCount> bytes from <buffer> into a new
 * frame and returning the frame-number
 *
 * @param buffer the buffer
 * @param loadCount the number of bytes to copy
 * @param regFlags the flags of the affected region
 * @return the frame-number
 */
frameno_t paging_demandLoad(void *buffer,size_t loadCount,ulong regFlags);

/**
 * Copies the memory (size=PAGE_SIZE) from <src> into the given frame.
 *
 * @param frame the destination-frame
 * @param src the source (in the current address-space, readable)
 */
void paging_copyToFrame(frameno_t frame,const void *src);

/**
 * Copies the memory (size=PAGE_SIZE) from the given frame to <dst>
 *
 * @param frame the source-frame
 * @param dst the destination (in the current address-space, writable)
 */
void paging_copyFromFrame(frameno_t frame,void *dst);

/**
 * Copies <count> bytes from <src> to <dst> in user-space.
 * The destination memory might not be writable!
 *
 * @param dst the destination address
 * @param src the source address
 * @param count the number of bytes to copy
 */
void paging_copyToUser(void *dst,const void *src,size_t count);

/**
 * Copies <count> zeros to <dst>, which is in user-space.
 * The destination memory might not be writable!
 *
 * @param dst the destination address
 * @param count the number of bytes to copy
 */
void paging_zeroToUser(void *dst,size_t count);

/**
 * Clones <count> pages at <virtSrc> to <virtDst> from <src> into <dst>. That means
 * the flags are copied. If <share> is true, the frames will be copied as well. Otherwise new
 * frames will be allocated if the page in <src> is present. Additionally, if <share> is false
 * all present pages will be marked as not-writable and share the frame in the cloned pagedir
 * so that they can be copied on write-access. Note that either <src> or <dst> has to be the
 * current page-dir!
 *
 * @param src the source-pagedir
 * @param dst the destination-pagedir
 * @param virtSrc the virtual source address
 * @param virtDst the virtual destination address
 * @param count the number of pages to copy
 * @param share whether to share the frames
 * @return the number of mapped frames (not necessarily new allocated), and allocated ptables
 */
sAllocStats paging_clonePages(tPageDir *src,tPageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,
		size_t count,bool share);

/**
 * Maps <count> pages starting at <virt> to the given frames in the CURRENT page-directory.
 * Note that the frame-number will just be set (and thus, <frames> used) when the flag PG_PRESENT
 * is specified.
 *
 * @param virt the virt start-address
 * @param frames an array with <count> elements which contains the frame-numbers to use.
 * 	a NULL-value causes the function to request MM_DEF-frames from mm on its own!
 * @param count the number of pages to map
 * @param flags some flags for the pages (PG_*)
 * @return the number of allocated frames and page-tables
 */
sAllocStats paging_map(uintptr_t virt,const frameno_t *frames,size_t count,uint flags);

/**
 * Maps <count> pages starting at <virt> to the given frames in the given page-directory.
 * Note that the frame-number will just be set (and thus, <frames> used) when the flag PG_PRESENT
 * is specified.
 *
 * @param pdir the physical address of the page-directory
 * @param virt the virt start-address
 * @param frames an array with <count> elements which contains the frame-numbers to use.
 * 	a NULL-value causes the function to request MM_DEF-frames from mm on its own!
 * @param count the number of pages to map
 * @param flags some flags for the pages (PG_*)
 * @return the number of allocated frames and page-tables
 */
sAllocStats paging_mapTo(tPageDir *pdir,uintptr_t virt,const frameno_t *frames,size_t count,uint flags);

/**
 * Removes <count> pages starting at <virt> from the page-tables in the CURRENT page-directory.
 * If you like the function free's the frames.
 * Note that the function will delete page-tables, too, if they are empty!
 *
 * @param virt the virtual start-address
 * @param count the number of pages to unmap
 * @param freeFrames whether the frames should be free'd and not just unmapped
 * @return the number of free'd frames and ptables
 */
sAllocStats paging_unmap(uintptr_t virt,size_t count,bool freeFrames);

/**
 * Removes <count> pages starting at <virt> from the page-tables in the given page-directory.
 * If you like the function free's the frames.
 * Note that the function will delete page-tables, too, if they are empty!
 *
 * @param pdir the physical address of the page-directory
 * @param virt the virtual start-address
 * @param count the number of pages to unmap
 * @param freeFrames whether the frames should be free'd and not just unmapped
 * @return the number of free'd frames and ptables
 */
sAllocStats paging_unmapFrom(tPageDir *pdir,uintptr_t virt,size_t count,bool freeFrames);

/**
 * Determines the number of page-tables (in the user-area) in the given page-directory
 *
 * @param pdir the page-directory
 * @return the number of present page-tables
 */
size_t paging_getPTableCount(tPageDir *pdir);

/**
 * Prints the user-part of the given page-directory to the given buffer
 *
 * @param buffer the buffer
 * @param pdir the page-directory
 */
void paging_sprintfVirtMem(sStringBuffer *buf,tPageDir *pdir);

/**
 * Prints the given parts of the current page-directory
 *
 * @param parts the parts to print
 */
void paging_printCur(uint parts);

/**
 * Prints the given parts from the given page-directory
 *
 * @param pdir the page-directory
 * @param parts the parts to print
 */
void paging_printPDir(tPageDir *pdir,uint parts);

/**
 * Counts the number of pages that are currently present in the given page-directory
 *
 * @param pdir the page-directory
 * @return the number of pages
 */
size_t paging_dbg_getPageCount(void);

/**
 * Prints the page at given virtual address
 *
 * @param pdir the page-directory
 * @param virt the virtual address
 */
void paging_printPageOf(tPageDir *pdir,uintptr_t virt);

#endif /*PAGING_H_*/
