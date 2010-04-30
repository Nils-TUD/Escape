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

#ifndef REGUSE_H_
#define REGUSE_H_

#include <common.h>
#include <mem/region.h>
#include <task/proc.h>
#include <task/thread.h>
#include <printf.h>

#define DISABLE_DEMLOAD		1

#define MAX_REGUSE_COUNT	(PROC_COUNT * 8)

#define RNO_TEXT			0
#define RNO_RODATA			1
#define RNO_BSS				2
#define RNO_DATA			3

#define REG_TEXT			0
#define REG_RODATA			1
#define REG_DATA			2
#define REG_BSS				3
#define REG_STACK			4
#define REG_SHM				5
#define REG_PHYS			6
#define REG_TLS				7
#define REG_SHLIBTEXT		8
#define REG_SHLIBDATA		9
#define REG_SHLIBBSS		10
#define REG_DLDATA			11

typedef struct {
	sRegion *reg;
	u32 virt;
} sVMRegion;

/**
 * Initializes the virtual memory management
 */
void vmm_init(void);

/**
 * Adds a region for physical memory mapped into the virtual memory (e.g. vga text-mode).
 * Please use this function instead of vmm_add() because this one maps the pages!
 *
 * @param p the process
 * @param phys the physical memory to map
 * @param bCount the number of bytes to map
 * @return the address or 0 if failed
 */
u32 vmm_addPhys(sProc *p,u32 phys,u32 bCount);

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
 * @param p the process
 * @param bin the binary (may be NULL; in this case no demand-loading is supported)
 * @param binOffset the offset in the binary from where to load the region (ignored if bin is NULL)
 * @param bCount the number of bytes
 * @param type the type of region
 * @return the region-number on success or a negative error-code
 */
tVMRegNo vmm_add(sProc *p,sBinDesc *bin,u32 binOffset,u32 bCount,u8 type);

/**
 * Tests wether the region with given number exists
 *
 * @param p the process
 * @param reg the region-number
 * @return true if so
 */
bool vmm_exists(sProc *p,tVMRegNo reg);

/**
 * Searches for the dynamic-link-data-region (growable)
 *
 * @param p the process
 * @return the region-number of -1 if not found
 */
tVMRegNo vmm_getDLDataReg(sProc *p);

/**
 * Determines the memory-usage of the given process
 *
 * @param p the process
 * @param paging the number of frames allocated for paging-structures
 * @param data the number of data-pages
 */
void vmm_getMemUsage(sProc *p,u32 *paging,u32 *data);

/**
 * Queries the start- and end-address of a region
 *
 * @param p the process
 * @param reg the region-number
 * @param start will be set to the start-address
 * @param end will be set to the end-address (exclusive; i.e. 0x1000 means 0xfff is the last
 *  accessible byte)
 */
void vmm_getRegRange(sProc *p,tVMRegNo reg,u32 *start,u32 *end);

/**
 * Checks wether the given process has the given binary as text-region
 *
 * @param p the process
 * @param bin the binary
 * @return the region-number if so or -1
 */
tVMRegNo vmm_hasBinary(sProc *p,sBinDesc *bin);

/**
 * Tries to handle a page-fault for the given address. That means, loads a page on demand, zeros
 * it on demand, handles copy-on-write or swapping.
 *
 * @param addr the address that caused the page-fault
 * @return true if successfull
 */
bool vmm_pagefault(u32 addr);

/**
 * Removes the given region from the given process
 *
 * @param p the process
 * @param reg the region-number
 */
void vmm_remove(sProc *p,tVMRegNo reg);

/**
 * Joins process <dst> to the region <rno> of process <src>. This can just be used for shared-
 * memory!
 *
 * @param src the source-process
 * @param rno the region-number in the source-process
 * @param dst the destination-process
 * @return the region-number on success or the negative error-code
 */
tVMRegNo vmm_join(sProc *src,tVMRegNo rno,sProc *dst);

/**
 * Clones all regions of the current process into the given one
 *
 * @param dst the destination-process
 * @return 0 on success
 */
s32 vmm_cloneAll(sProc *dst);

/**
 * Grows the stack of the given thread so that <addr> is accessible, if possible
 *
 * @param t the thread
 * @param addr the address
 * @return 0 on success
 */
s32 vmm_growStackTo(sThread *t,u32 addr);

/**
 * If <amount> is positive, the region will be grown by <amount> pages. If negative it will be
 * shrinked. If 0 it returns the current offset to the region-beginning, in pages.
 *
 * @param p the process
 * @param reg the region-number
 * @param amount the number of pages to add/remove
 * @return the old offset to the region-beginning, in pages
 */
s32 vmm_grow(sProc *p,tVMRegNo reg,s32 amount);

/**
 * Prints information about all regions in the given process to the given buffer
 *
 * @param buf the buffer
 * @param p the process
 */
void vmm_sprintfRegions(sStringBuffer *buf,sProc *p);

#if DEBUGGING

/**
 * Prints all regions of the given process
 *
 * @param p the process
 */
void vmm_dbg_print(sProc *p);

#endif

#endif /* REGUSE_H_ */
