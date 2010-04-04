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

#include <common.h>
#include <printf.h>
#include <sllist.h>

#define PF_COPYONWRITE		1
#define PF_DEMANDLOAD		2
#define PF_DEMANDZERO		4
#define PF_SWAPPED			8
#define PF_LOADINPROGRESS	16

#define RF_GROWABLE			1
#define RF_SHAREABLE		2
#define RF_WRITABLE			4
#define RF_STACK			8	/* means, grows downwards, and is used to find a free stack-address */

typedef struct {
	const char *path;
	u32 modifytime;
} sBinDesc;

typedef struct {
	u32 flags;			/* flags that specify the attributes of this region */
	sBinDesc binary;	/* the source-binary (for demand-paging) */
	u32 binOffset;		/* offset in the binary */
	u32 byteCount;		/* number of bytes */
	u32 pfSize;			/* size of pageFlags */
	u8 *pageFlags;		/* flags for each page */
	sSLList *procs;		/* linked list of processes that use this region */
} sRegion;

/**
 * Creates a new region with given attributes. Note that the path in <bin> will be copied
 * to the heap. Initially the process-collection that use the region will be empty!
 *
 * @param bin the binary (may be NULL)
 * @param binOffset the offset in the binary (ignored if bin is NULL)
 * @param bCount the number of bytes
 * @param pgFlags the flags of the pages (PF_*)
 * @param flags the flags of the region (RF_*)
 * @return the region or NULL if failed
 */
sRegion *reg_create(sBinDesc *bin,u32 binOffset,u32 bCount,u8 pgFlags,u32 flags);

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
u32 reg_presentPageCount(sRegion *reg);

/**
 * @param reg the region
 * @return the number of references of the given region
 */
u32 reg_refCount(sRegion *reg);

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
bool reg_grow(sRegion *reg,s32 amount);

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
sRegion *reg_clone(const void *p,sRegion *reg);

/**
 * Prints information about the given region in the given buffer
 *
 * @param buf the buffer
 * @param reg the region
 */
void reg_sprintf(sStringBuffer *buf,sRegion *reg);

#endif /* REGION_H_ */
