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
#include <sys/printf.h>
#include <esc/sllist.h>

#define PF_BITCOUNT			3		/* number of bits occupied by real flags */
#define PF_COPYONWRITE		1UL
#define PF_DEMANDLOAD		2UL
#define PF_SWAPPED			4UL

#define RF_GROWABLE			1UL
#define RF_SHAREABLE		2UL
#define RF_WRITABLE			4UL
#define RF_EXECUTABLE		8UL
#define RF_STACK			16UL
#define RF_NOFREE			32UL	/* means that the memory should not be free'd on release */
#define RF_TLS				64UL	/* needed to distinguish TLS-regions from others */
#define RF_GROWS_DOWN		128UL

typedef struct {
	ulong flags;			/* flags that specify the attributes of this region */
	sFile *file;			/* NULL if the region is not backed by a file */
	off_t offset;			/* offset in the binary */
	size_t loadCount;		/* number of bytes to load from file */
	size_t byteCount;		/* number of bytes */
	uint64_t timestamp;		/* timestamp of last usage (for swapping) */
	size_t pfSize;			/* size of pageFlags */
	ulong *pageFlags;		/* flags for each page; upper bits: swap-block, if swapped */
	sSLList procs;			/* linked list of processes that use this region */
	mutex_t lock;			/* lock for the procs-field (all others can't change or belong to
	 	 	 	 	 	 	   exactly 1 process, which is locked anyway) */
} sRegion;

/**
 * Creates a new region with given attributes. Initially the process-collection that use the
 * region will be empty!
 *
 * @param file the file
 * @param bCount the number of bytes
 * @param lCount the number of bytes to load from file
 * @param offset the offset in the binary (ignored if bin is NULL)
 * @param pgFlags the flags of the pages (PF_*)
 * @param flags the flags of the region (RF_*)
 * @return the region or NULL if failed
 */
sRegion *reg_create(sFile *file,size_t bCount,size_t lCount,size_t offset,ulong pgFlags,ulong flags);

/**
 * Destroys the given region (regardless of the number of users!)
 *
 * @param reg the region
 */
void reg_destroy(sRegion *reg);

/**
 * Counts the number of present and swapped out pages in the given region
 *
 * @param reg the region
 * @param swapped will be set to the number of swapped out pages
 * @return the number of present pages
 */
size_t reg_pageCount(const sRegion *reg,size_t *swapped);

/**
 * @param reg the region
 * @return the number of references of the given region
 */
size_t reg_refCount(sRegion *reg);

/**
 * Returns the swap-block in which the page with given index is stored
 *
 * @param reg the region
 * @param pageIndex the index of the page in the region
 * @return the swap-block
 */
ulong reg_getSwapBlock(const sRegion *reg,size_t pageIndex);

/**
 * Sets the swap-block in which the page with given index is stored to <swapBlock>.
 *
 * @param reg the region
 * @param pageIndex the index of the page in the region
 * @param swapBlock the swap-block
 */
void reg_setSwapBlock(sRegion *reg,size_t pageIndex,ulong swapBlock);

/**
 * Adds the given process as user to the region
 *
 * @param reg the region
 * @param p the process
 * @return true if successfull
 */
bool reg_addTo(sRegion *reg,const void *p);

/**
 * Removes the given process as user from the region
 *
 * @param reg the region
 * @param p the process
 * @return true if found
 */
bool reg_remFrom(sRegion *reg,const void *p);

/**
 * Grows/shrinks the given region by <amount> pages. The added page-flags are always 0.
 * Note that the stack grows downwards, all other regions upwards.
 *
 * @param reg the region
 * @param amount the number of pages
 * @return the number of free'd swapped pages or a negative error-code
 */
ssize_t reg_grow(sRegion *reg,ssize_t amount);

/**
 * Clones the given region for the given process. That means it copies the attributes from the
 * given region into a new one and puts <p> as the only user into it. It assumes that <reg>
 * has just one user, too (since shared regions can't be cloned).
 * The page-flags are simply copied, i.e. you have to handle copy-on-write!
 *
 * @param p the process
 * @param reg the region
 * @return the created region or NULL if failed
 */
sRegion *reg_clone(const void *p,const sRegion *reg);

/**
 * Prints information about the given region in the given buffer
 *
 * @param buf the buffer
 * @param reg the region
 * @param virt the virtual-address at which the region is mapped
 */
void reg_sprintf(sStringBuffer *buf,sRegion *reg,uintptr_t virt);

/**
 * Prints the given region
 *
 * @param reg the region
 * @param virt the virtual-address at which the region is mapped
 */
void reg_print(sRegion *reg,uintptr_t virt);

/**
 * Prints the flags of the given region
 *
 * @param reg the region
 */
void reg_printFlags(const sRegion *reg);

/**
 * Prints the flags of the given region into the given buffer
 *
 * @param buf the buffer
 * @param reg the region
 */
void reg_sprintfFlags(sStringBuffer *buf,const sRegion *reg);
