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
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/printf.h>

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
 * 0xA0000000: +-----------------------------------+     |
 *             |  dynlinker (always first, if ex)  |     |
 *             |           shared memory           |     |
 *             |         shared libraries          |     |
 *             |          shared lib data          |     |
 *             |       thread local storage        |     |
 *             |        memory mapped stuff        |     |
 *             |        (in arbitrary order)       |     |
 * 0xC0000000: +-----------------------------------+   -----
 *             |                ...                |     |
 * 0xC0100000: +-----------------------------------+     |
 *             |         kernel code+data          |     |
 *             +-----------------------------------+
 *      |      |             mm-stack              |     k
 *      v      +-----------------------------------+     e
 *             |                ...                |     r
 * 0xC0800000: +-----------------------------------+     n
 *             |                                   |     e
 *      |      |            kernel-heap            |     l
 *      v      |                                   |     a
 * 0xC1C00000: +-----------------------------------+     r
 *             |           temp map area           |     e
 * 0xC2800000: +-----------------------------------+     a
 *             |                ...                |
 * 0xFF7FF000: +-----------------------------------+     |      -----
 *             |            kernel-stack           |     |        |
 * 0xFF800000: +-----------------------------------+     |
 *             |     temp mapped page-tables       |     |    not shared page-tables (3)
 * 0xFFC00000: +-----------------------------------+     |
 *             |        mapped page-tables         |     |        |
 * 0xFFFFFFFF: +-----------------------------------+   -----    -----
 */

/* the virtual address of the kernel-area */
#define KERNEL_AREA_V_ADDR		((u32)0xC0000000)
/* the virtual address of the kernel itself */
#define KERNEL_V_ADDR			(KERNEL_AREA_V_ADDR + KERNEL_P_ADDR)

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT			(PAGE_SIZE / 4)

/* the start of the mapped page-tables area */
#define MAPPED_PTS_START		(0xFFFFFFFF - (PT_ENTRY_COUNT * PAGE_SIZE) + 1)
/* the start of the temporary mapped page-tables area */
#define TMPMAP_PTS_START		(MAPPED_PTS_START - (PT_ENTRY_COUNT * PAGE_SIZE))
/* the start of the kernel-heap */
#define KERNEL_HEAP_START		(KERNEL_AREA_V_ADDR + (PT_ENTRY_COUNT * PAGE_SIZE) * 2)
/* the size of the kernel-heap (4 MiB) */
#define KERNEL_HEAP_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE /* * 4 */)

/* page-directories in virtual memory */
#define PAGE_DIR_AREA			(MAPPED_PTS_START + PAGE_SIZE * (PT_ENTRY_COUNT - 1))
/* needed for building a new page-dir */
#define PAGE_DIR_TMP_AREA		(TMPMAP_PTS_START + PAGE_SIZE * (PT_ENTRY_COUNT - 1))
/* our kernel-stack */
#define KERNEL_STACK			(TMPMAP_PTS_START - PAGE_SIZE)

/* for mapping some pages of foreign processes */
#define TEMP_MAP_AREA			(KERNEL_HEAP_START + KERNEL_HEAP_SIZE)
#define TEMP_MAP_AREA_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE * 4)

/* the size of the temporary stack we use at the beginning */
#define TMP_STACK_SIZE			PAGE_SIZE

/* flags for paging_map() */
#define PG_WRITABLE				1
#define PG_SUPERVISOR			2
#define PG_PRESENT				4
/* tells paging_map() that it gets the frame-address and should convert it to a frame-number first */
#define PG_ADDR_TO_FRAME		8
/* make it a global page */
#define PG_GLOBAL				16
/* tells paging_map() to keep the currently set frame-number */
#define PG_KEEPFRM				32

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr)	((u32)(addr) / PAGE_SIZE / PT_ENTRY_COUNT)

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr)	(((u32)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT)

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount)	(((u32)(pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

/* for printing the page-directory */
#define PD_PART_ALL				0
#define PD_PART_USER 			1
#define PD_PART_KERNEL			2
#define PD_PART_KHEAP			4
#define PD_PART_PTBLS			8
#define PD_PART_TEMPMAP			16

/* start-address of the text */
#define TEXT_BEGIN				0x1000
/* start-address of the text in dynamic linker */
#define INTERP_TEXT_BEGIN		0xA0000000

typedef struct {
	u32 ptables;
	u32 frames;
} sAllocStats;

/**
 * Inits the paging. Sets up the page-dir and page-tables for the kernel and enables paging
 */
void paging_init(void);

/**
 * Sets the current page-dir. Has to be called on every context-switch!
 *
 * @param pdir the pagedir
 */
void paging_setCur(tPageDir pdir);

/**
 * Reserves page-tables for the whole higher-half and inserts them into the page-directory.
 * This should be done ONCE at the beginning as soon as the physical memory management is set up
 */
void paging_mapKernelSpace(void);

/**
 * Unmaps the page-table 0. This should be used only by the GDT to unmap the first page-table as
 * soon as the GDT is setup for a flat memory layout!
 */
void paging_gdtFinished(void);

/**
 * Note that this should just be used by proc_init()!
 *
 * @return the address of the page-directory of process 0
 */
tPageDir paging_getProc0PD(void);

/**
 * Assembler routine to exchange the page-directory to the given one
 *
 * @param physAddr the physical address of the page-directory
 */
extern void paging_exchangePDir(tPageDir physAddr);

/**
 * Checks whether the given address-range is currently readable for the user
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserReadable(u32 virt,u32 count);

/**
 * Checks whether the given address-range is currently readable
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeReadable(u32 virt,u32 count);

/**
 * Checks whether the given address-range is currently writable for the user.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserWritable(u32 virt,u32 count);

/**
 * Checks whether the given address-range is currently writable.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeWritable(u32 virt,u32 count);

/**
 * Maps the given frames (frame-numbers) to a temporary area (writable, super-visor), so that you
 * can access it. Please use paging_unmapFromTemp() as soon as you're finished!
 *
 * @param frames the frame-numbers
 * @param count the number of frames
 * @return the virtual start-address
 */
u32 paging_mapToTemp(u32 *frames,u32 count);

/**
 * Unmaps the temporary mappings
 *
 * @param count the number of pages
 */
void paging_unmapFromTemp(u32 count);

/**
 * Clones the kernel-space of the current page-dir into a new one.
 *
 * @param stackFrame will be set to the used kernel-stackframe
 * @param pdir will be set to the page-directory address (physical)
 * @return the number of allocated frames
 */
s32 paging_cloneKernelspace(u32 *stackFrame,tPageDir *pdir);

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
bool paging_isPresent(tPageDir pdir,u32 virt);

/**
 * Returns the frame-number of the given virtual address in the given pagedir. Assumes that
 * its present.
 *
 * @param pdir the page-dir
 * @param virt the virtual address
 * @return the frame-number of the given virtual address
 */
u32 paging_getFrameNo(tPageDir pdir,u32 virt);

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
sAllocStats paging_clonePages(tPageDir src,tPageDir dst,u32 virtSrc,u32 virtDst,u32 count,bool share);

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
sAllocStats paging_map(u32 virt,u32 *frames,u32 count,u8 flags);

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
sAllocStats paging_mapTo(tPageDir pdir,u32 virt,u32 *frames,u32 count,u8 flags);

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
sAllocStats paging_unmap(u32 virt,u32 count,bool freeFrames);

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
sAllocStats paging_unmapFrom(tPageDir pdir,u32 virt,u32 count,bool freeFrames);

/**
 * Prints the user-part of the given page-directory to the given buffer
 *
 * @param buffer the buffer
 * @param pdir the page-directory
 */
void paging_sprintfVirtMem(sStringBuffer *buf,tPageDir pdir);


#if DEBUGGING

/**
 * Counts the number of pages that are currently present in the given page-directory
 *
 * @param pdir the page-directory
 * @return the number of pages
 */
u32 paging_dbg_getPageCount(void);

/**
 * Prints the page at given virtual address
 *
 * @param pdir the page-directory
 * @param virt the virtual address
 */
void paging_dbg_printPageOf(tPageDir pdir,u32 virt);

/**
 * Prints the given parts of the current page-directory
 *
 * @param parts the parts to print
 */
void paging_dbg_printCur(u8 parts);

/**
 * Prints the given parts from the given page-directory
 *
 * @param pdir the page-directory
 * @param parts the parts to print
 */
void paging_dbg_printPDir(tPageDir pdir,u8 parts);

#endif

#endif /*PAGING_H_*/
