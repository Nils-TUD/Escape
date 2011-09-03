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

#ifndef REGUSE_H_
#define REGUSE_H_

#include <sys/common.h>
#include <sys/mem/region.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/printf.h>

#define DISABLE_DEMLOAD		0

#define MAX_REGUSE_COUNT	8192

#define RNO_TEXT			0
#define RNO_RODATA			1
#define RNO_DATA			2

#define REG_TEXT			0
#define REG_RODATA			1
#define REG_DATA			2
#define REG_STACK			3
#define REG_STACKUP			4
#define REG_SHM				5
#define REG_DEVICE			6
#define REG_TLS				7
#define REG_SHLIBTEXT		8
#define REG_SHLIBDATA		9
#define REG_DLDATA			10
#define REG_PHYS			11

typedef struct {
	sRegion *reg;
	uintptr_t virt;
	/* file for the binary (valid if >= 0) */
	file_t binFile;
} sVMRegion;

/**
 * Initializes the virtual memory management
 */
void vmm_init(void);

/**
 * Adds a region for physical memory mapped into the virtual memory (e.g. for vga text-mode or DMA).
 * Please use this function instead of vmm_add() because this one maps the pages!
 *
 * @param pid the process-id
 * @param phys a pointer to the physical memory to map; if *phys is 0, the function allocates
 * 	contiguous physical memory itself and stores the address in *phys
 * @param bCount the number of bytes to map
 * @param align the alignment for the allocated physical-memory (just if *phys = 0)
 * @return the virtual address or 0 if failed
 */
uintptr_t vmm_addPhys(pid_t pid,uintptr_t *phys,size_t bCount,size_t align);

/**
 * Adds a region of given type to the given process. Note that you can't add regions in arbitrary
 * order. Text, rodata, bss and data are put in the VM in this order and can therefore only be
 * created in this order (but you can leave out regions; e.g. you can add text, bss and data).
 * Stack, shared-memory and physical memory can be added in arbitrary order.
 *
 * If the region comes from a binary (text, data, ...), you have to provide the binary because
 * it will be loaded on demand.
 *
 * Note: Please use vmm_addPhys() when adding a region that maps physical memory!
 *
 * @param pid the process-id
 * @param bin the binary (may be NULL; in this case no demand-loading is supported)
 * @param binOffset the offset in the binary from where to load the region (ignored if bin is NULL)
 * @param bCount the number of bytes
 * @param lCount number of bytes to load from disk (the rest is zero'ed)
 * @param type the type of region
 * @return the region-number on success or a negative error-code
 */
vmreg_t vmm_add(pid_t pid,const sBinDesc *bin,off_t binOffset,size_t bCount,size_t lCount,uint type);

/**
 * Changes the protection-settings of the given region. This is not possible for TLS-, stack-
 * and nofree-regions.
 *
 * @param pid the process-id
 * @param rno the region-number
 * @param flags the new flags (RF_WRITABLE or 0)
 * @return 0 on success
 */
int vmm_setRegProt(pid_t pid,vmreg_t rno,ulong flags);

/**
 * Swaps the page at given index in given region out. I.e. it marks the page as swapped in the
 * region and unmaps it from all affected processes.
 * Does NOT free the frame!
 *
 * @param reg the region
 * @param index the page-index in the region
 */
void vmm_swapOut(sRegion *reg,size_t index);

/**
 * Swaps the page at given index in given region in. I.e. it marks the page as not-swapped in
 * the region and maps it with given frame-number for all affected processes.
 *
 * @param reg tje region
 * @param index the page-index in the region
 * @param frameNo the frame-number
 */
void vmm_swapIn(sRegion *reg,size_t index,frameno_t frameNo);

/**
 * Sets the timestamp for all regions that are used by the given thread
 *
 * @param t the thread
 * @param timestamp the timestamp to set
 */
void vmm_setTimestamp(const sThread *t,uint64_t timestamp);

/**
 * Returns the least recently used region of the given process that contains swappable pages
 *
 * @param pid the process-id
 * @return the LRU region (may be NULL)
 */
sRegion *vmm_getLRURegion(pid_t pid);

/**
 * Returns a random page-index in the given region that may be swapped out
 *
 * @param reg the region
 * @return the page-index (-1 if failed)
 */
size_t vmm_getPgIdxForSwap(const sRegion *reg);

/**
 * Tests whether the region with given number exists
 *
 * @param pid the process-id
 * @param reg the region-number
 * @return true if so
 */
bool vmm_exists(pid_t pid,vmreg_t reg);

/**
 * Searches for the dynamic-link-data-region (growable)
 *
 * @param pid the process-id
 * @return the region-number of -1 if not found
 */
vmreg_t vmm_getDLDataReg(pid_t pid);

/**
 * Sets the given pointers to the corresponding number of frames for the given process
 *
 * @param pid the process-id
 * @param own will be set to the number of own frames
 * @param shared will be set to the number of shared frames
 * @param swapped will be set to the number of swapped frames
 */
void vmm_getMemUsageOf(pid_t pid,size_t *own,size_t *shared,size_t *swapped);

/**
 * This is a helper-function for determining the real memory-usage of all processes. It counts
 * the number of present frames in all regions of the given process and divides them for each
 * region by the number of region-users. It does not count cow-pages!
 * This way, we don't count shared regions multiple times (at the end the division sums up to
 * one usage of the region).
 *
 * @param pid the process-id
 * @param pages will point to the number of pages (size of virtual-memory)
 * @return the number of used frames for this process
 */
float vmm_getMemUsage(pid_t pid,size_t *pages);

/**
 * @param pid the process-id
 * @param rno the region-number
 * @return the vm-region with given number
 */
sVMRegion *vmm_getRegion(pid_t pid,vmreg_t rno);

/**
 * @param pid the process-id
 * @param addr the virtual address
 * @return the region-number to which the given virtual address belongs
 */
vmreg_t vmm_getRegionOf(pid_t pid,uintptr_t addr);

/**
 * Determines the vm-region-number of the given region for the given process
 *
 * @param p the process
 * @param reg the region
 * @return the vm-region-number or -1 if not found
 */
vmreg_t vmm_getRNoByRegion(pid_t pid,const sRegion *reg);

/**
 * Queries the start- and end-address of a region
 *
 * @param pid the process-id
 * @param reg the region-number
 * @param start will be set to the start-address
 * @param end will be set to the end-address (exclusive; i.e. 0x1000 means 0xfff is the last
 *  accessible byte)
 * @return true if the region exists
 */
bool vmm_getRegRange(pid_t pid,vmreg_t reg,uintptr_t *start,uintptr_t *end);

/**
 * Checks whether the given process has the given binary as text-region
 *
 * @param pid the process-id
 * @param bin the binary
 * @return the region-number if so or -1
 */
vmreg_t vmm_hasBinary(pid_t pid,const sBinDesc *bin);

/**
 * Ensures that <size> bytes from <src> to <dst> can be copied. That means, copy-on-write,
 * demand-loading and so on are handled, if necessary. This way its safe to copy to a page that
 * still has copy-on-write enabled, because some architectures allow it to write to readonly-pages
 * in kernel-mode.
 * NOTE: The function assumes that the region-lock of the current process is held!
 *
 * @param p the current process (locked)
 * @param dst the destination-address (in user-space)
 * @param size the number of bytes to copy
 * @return true if successfull
 */
bool vmm_makeCopySafe(sProc *p,USER void *dst,size_t size);

/**
 * Tries to handle a page-fault for the given address. That means, loads a page on demand, zeros
 * it on demand, handles copy-on-write or swapping.
 *
 * @param addr the address that caused the page-fault
 * @return true if successfull
 */
bool vmm_pagefault(uintptr_t addr);

/**
 * Removes all regions of the given process, optionally including stack.
 *
 * @param pid the process-id
 * @param remStack whether to remove the stack too
 */
void vmm_removeAll(pid_t pid,bool remStack);

/**
 * Removes the given region from the given process
 *
 * @param pid the process-id
 * @param reg the region-number
 */
void vmm_remove(pid_t pid,vmreg_t reg);

/**
 * Joins process <dst> to the region <rno> of process <src>. This can just be used for shared-
 * memory!
 *
 * @param srcId the source-process
 * @param rno the region-number in the source-process
 * @param dstId the destination-process
 * @return the region-number on success or the negative error-code
 */
vmreg_t vmm_join(pid_t srcId,vmreg_t rno,pid_t dstId);

/**
 * Clones all regions of the current process into the destination-process
 *
 * @param dstId the destination-process-id
 * @return 0 on success
 */
int vmm_cloneAll(pid_t dstId);

/**
 * Grows the stack of the given thread so that <addr> is accessible, if possible
 *
 * @param pid the process-id
 * @param reg the region
 * @param addr the address
 * @return 0 on success
 */
int vmm_growStackTo(pid_t pid,vmreg_t reg,uintptr_t addr);

/**
 * If <amount> is positive, the region will be grown by <amount> pages. If negative it will be
 * shrinked. If 0 it returns the current offset to the region-beginning, in pages.
 *
 * @param pid the process-id
 * @param reg the region-number
 * @param amount the number of pages to add/remove
 * @return the old offset to the region-beginning, in pages
 */
ssize_t vmm_grow(pid_t pid,vmreg_t reg,ssize_t amount);

/**
 * Prints information about all regions in the given process to the given buffer
 *
 * @param buf the buffer
 * @param pid the process-id
 */
void vmm_sprintfRegions(sStringBuffer *buf,pid_t pid);

/**
 * Prints a short version of the regions of given process
 *
 * @param pid the process-id
 */
void vmm_printShort(pid_t pid);

/**
 * Prints all regions of the given process
 *
 * @param pid the process-id
 */
void vmm_print(pid_t pid);

#endif /* REGUSE_H_ */
