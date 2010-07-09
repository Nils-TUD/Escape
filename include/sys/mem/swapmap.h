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

#ifndef SWAPMAP_H_
#define SWAPMAP_H_

#include <sys/common.h>
#include <sllist.h>

#define INVALID_BLOCK		0xFFFFFFFF

/**
 * Inits the swap-map
 *
 * @param swapSize the size of the swap-device in bytes
 */
void swmap_init(u32 swapSize);

/**
 * Removes all areas of the given process (that of course are not needed anymore by anyone else)
 * IMPORTANT: You HAVE TO call this function before you remove a process from the text-sharing-
 * or shared-memory-list!
 *
 * @param pid the process-id
 * @param procs the list of processes that use the area (this HAS TO be the list of shm, text-sharing
 * 	or NULL to remove all stuff that is owned by the process)
 */
void swmap_remProc(tPid pid,sSLList *procs);

/**
 * Allocates a place in the swap-device for the given virtual-memory-part
 *
 * @param pid the pid
 * @param procs a linked lists with processes that use this area (NULL if just <pid>)
 * @param virt the virtual address
 * @param count the number of pages to swap
 * @return the starting block on the swap-device or INVALID_BLOCK if no free space is left
 */
u32 swmap_alloc(tPid pid,sSLList *procs,u32 virt,u32 count);

/**
 * Searches for the block on the swap-device which has the content of the given page
 *
 * @param pid the pid
 * @param virt the virtual address of the page
 * @return the block of it or INVALID_BLOCK
 */
u32 swmap_find(tPid pid,u32 virt);

/**
 * Determines the free space in the swapmap
 *
 * @return the free space in bytes
 */
u32 swmap_freeSpace(void);

/**
 * Free's the given blocks owned by the given process
 *
 * @param pid the pid
 * @param block the starting block
 * @param count the number of blocks
 */
void swmap_free(tPid pid,u32 block,u32 count);


#if DEBUGGING

/**
 * Prints the swap-map
 */
void swmap_dbg_print(void);

#endif

#endif /* SWAPMAP_H_ */
