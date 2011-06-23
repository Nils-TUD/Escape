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

#ifndef REGION_H_
#define REGION_H_

#include <sys/common.h>
#include <sys/printf.h>
#include <esc/sllist.h>

#define PF_BITCOUNT			4	/* number of bits occupied by real flags */
#define PF_COPYONWRITE		1
#define PF_DEMANDLOAD		2
#define PF_SWAPPED			4
#define PF_LOADINPROGRESS	8

#define RF_GROWABLE			1
#define RF_SHAREABLE		2
#define RF_WRITABLE			4
#define RF_EXECUTABLE		8
#define RF_STACK			16	/* means, grows downwards, and is used to find a free stack-address */
#define RF_NOFREE			32	/* means that the memory should not be free'd on release */
#define RF_TLS				64	/* needed to distinguish TLS-regions from others on delete */
#define RF_GROWS_DOWN		128

typedef struct {
	tInodeNo ino;
	tDevNo dev;
	tTime modifytime;
} sBinDesc;

typedef struct {
	ulong flags;				/* flags that specify the attributes of this region */
	sBinDesc binary;		/* the source-binary (for demand-paging) */
	off_t binOffset;	/* offset in the binary */
	size_t byteCount;		/* number of bytes */
	size_t loadCount;		/* number of bytes to load from disk (the rest is zero'ed) */
	tTime timestamp;		/* timestamp of last usage (for swapping) */
	size_t pfSize;			/* size of pageFlags */
	ulong *pageFlags;		/* flags for each page; upper bits: swap-block, if swapped */
	sSLList *procs;			/* linked list of processes that use this region */
} sRegion;

/**
 * Creates a new region with given attributes. Note that the path in <bin> will be copied
 * to the heap. Initially the process-collection that use the region will be empty!
 *
 * @param bin the binary (may be NULL)
 * @param binOffset the offset in the binary (ignored if bin is NULL)
 * @param bCount the number of bytes
 * @param lCount number of bytes to load from disk (the rest is zero'ed)
 * @param pgFlags the flags of the pages (PF_*)
 * @param flags the flags of the region (RF_*)
 * @return the region or NULL if failed
 */
sRegion *reg_create(const sBinDesc *bin,off_t binOffset,size_t bCount,size_t lCount,
		ulong pgFlags,ulong flags);

/**
 * Destroys the given region (regardless of the number of users!)
 *
 * @param reg the region
 */
void reg_destroy(sRegion *reg);

/**
 * Counts the number of present pages in the given region
 *
 * @param reg the region
 * @return the number of present pages
 */
size_t reg_presentPageCount(const sRegion *reg);

/**
 * @param reg the region
 * @return the number of references of the given region
 */
size_t reg_refCount(const sRegion *reg);

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
 * @return true if successfull
 */
bool reg_grow(sRegion *reg,ssize_t amount);

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
void reg_sprintf(sStringBuffer *buf,const sRegion *reg,uintptr_t virt);

/**
 * Prints the flags of the given region
 *
 * @param reg the region
 */
void reg_printFlags(const sRegion *reg);

/**
 * Prints the given region
 *
 * @param reg the region
 * @param virt the virtual-address at which the region is mapped
 */
void reg_print(const sRegion *reg,uintptr_t virt);

#endif /* REGION_H_ */
