/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#define PG_WRITABLE				1
#define PG_SUPERVISOR			2
#define PG_PRESENT				4
/* TODO */
#define PG_EXECUTABLE			64
/* tells paging_map() that it gets the frame-address and should convert it to a frame-number first */
#define PG_ADDR_TO_FRAME		8
/* make it a global page */
#define PG_GLOBAL				16
/* tells paging_map() to keep the currently set frame-number */
#define PG_KEEPFRM				32

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
 * @return the current page dir
 */
tPageDir paging_getCur(void);

/**
 * Sets the current page-dir. Has to be called on every context-switch!
 *
 * @param pdir the pagedir
 */
void paging_setCur(tPageDir pdir);

/**
 * Checks whether the given address-range is currently readable for the user
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserReadable(uintptr_t virt,size_t count);

/**
 * Checks whether the given address-range is currently readable
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeReadable(uintptr_t virt,size_t count);

/**
 * Checks whether the given address-range is currently writable for the user.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserWritable(uintptr_t virt,size_t count);

/**
 * Checks whether the given address-range is currently writable.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeWritable(uintptr_t virt,size_t count);

/**
 * Maps the given frames (frame-numbers) to a temporary area (writable, super-visor), so that you
 * can access it. Please use paging_unmapFromTemp() as soon as you're finished!
 *
 * @param frames the frame-numbers
 * @param count the number of frames
 * @return the virtual start-address
 */
uintptr_t paging_mapToTemp(const tFrameNo *frames,size_t count);

/**
 * Unmaps the temporary mappings
 *
 * @param count the number of pages
 */
void paging_unmapFromTemp(size_t count);

/**
 * Clones the kernel-space of the current page-dir into a new one.
 *
 * @param stackFrame will be set to the used kernel-stackframe
 * @param pdir will be set to the page-directory address (physical)
 * @return the number of allocated frames
 */
ssize_t paging_cloneKernelspace(tFrameNo *stackFrame,tPageDir *pdir);

/**
 * Destroys the given page-directory (not the current!)
 *
 * @param pdir the page-dir
 * @return the number of free'd frames and ptables
 */
sAllocStats paging_destroyPDir(tPageDir pdir);

/**
 * Determines wether the given page is present
 *
 * @param pdir the page-dir
 * @param virt the virtual address
 * @return true if so
 */
bool paging_isPresent(tPageDir pdir,uintptr_t virt);

/**
 * Returns the frame-number of the given virtual address in the given pagedir. Assumes that
 * its present.
 *
 * @param pdir the page-dir
 * @param virt the virtual address
 * @return the frame-number of the given virtual address
 */
tFrameNo paging_getFrameNo(tPageDir pdir,uintptr_t virt);

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
 * @param share wether to share the frames
 * @return the number of mapped frames (not necessarily new allocated), and allocated ptables
 */
sAllocStats paging_clonePages(tPageDir src,tPageDir dst,uintptr_t virtSrc,uintptr_t virtDst,
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
sAllocStats paging_map(uintptr_t virt,const tFrameNo *frames,size_t count,uint flags);

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
sAllocStats paging_mapTo(tPageDir pdir,uintptr_t virt,const tFrameNo *frames,size_t count,uint flags);

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
sAllocStats paging_unmapFrom(tPageDir pdir,uintptr_t virt,size_t count,bool freeFrames);

/**
 * Determines the number of page-tables (in the user-area) in the given page-directory
 *
 * @param pdir the page-directory
 * @return the number of present page-tables
 */
size_t paging_getPTableCount(tPageDir pdir);

/**
 * Prints the user-part of the given page-directory to the given buffer
 *
 * @param buffer the buffer
 * @param pdir the page-directory
 */
void paging_sprintfVirtMem(sStringBuffer *buf,tPageDir pdir);

/**
 * Prints the given parts of the current page-directory
 *
 * @param parts the parts to print
 */
void paging_dbg_printCur(uint parts);

/**
 * Prints the given parts from the given page-directory
 *
 * @param pdir the page-directory
 * @param parts the parts to print
 */
void paging_dbg_printPDir(tPageDir pdir,uint parts);


#if DEBUGGING

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
void paging_dbg_printPageOf(tPageDir pdir,uintptr_t virt);

#endif

#endif /*PAGING_H_*/
