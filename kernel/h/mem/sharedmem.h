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

#ifndef SHAREDMEM_H_
#define SHAREDMEM_H_

#include <common.h>
#include <task/proc.h>

/**
 * Creates a shared-memory with given name and page-count
 *
 * @param name the name
 * @param pageCount the number of pages
 * @return the start-page on success
 */
s32 shm_create(const char *name,u32 pageCount);

/**
 * Checks whether the given addr of the given process belongs to a shared-memory region. If so
 * <pageCount> and <isOwner> is set to the corresponding values.
 *
 * @param p the process
 * @param addr the virtual address
 * @param pageCount will be set to the size of the area, if it is one
 * @param isOwner will be set to true if it is one and the process is the owner
 * @return true if it belongs to a shared-memory region
 */
bool shm_isSharedMem(sProc *p,u32 addr,u32 *pageCount,bool *isOwner);

/**
 * Determines the address of the corresponding page in the shared-memory-area specified by <p> and
 * <addr> in the other given process. That means if the area starts in <p> at 0x00001000, <addr>
 * is 0x00002000 and it starts in <other> at 0x00040000, the function returns 0x00041000.
 * This is needed if you want to manipulate a page of the shared-memory-area of another user
 * of this area.
 *
 * @param p the process
 * @param addr the virtual address in the shm-area of <p>
 * @param other the other process
 * @return the virtual address of that page in <other> or 0 if not found
 */
u32 shm_getAddrOfOther(sProc *p,u32 addr,sProc *other);

/**
 * Checks whether the given addr of the given process belongs to a shared-memory region. If so the
 * members are returned including the owner.
 *
 * @param owner the process
 * @param addr the virtual address
 * @return a linked list of the processes that use the region or NULL
 */
sSLList *shm_getMembers(sProc *owner,u32 addr);

/**
 * Joins the shared-memory with given name. Will copy the pages to the end of the data-segment
 *
 * @param name the name
 * @return the start-page on success
 */
s32 shm_join(const char *name);

/**
 * Leaves the shared-memory with given name. Will NOT unmap the pages!
 * The problem is that the shared-memory is in the data-segment of the process. The process may
 * have added additional data-pages behind the shared-memory. Since the data-segment must be
 * "one piece" we can't remove pages in the middle. So we do it never. I think that's not
 * really a problem because the shm-owner has trusted the process anyway.
 *
 * @param name the name
 * @return 0 on success
 */
s32 shm_leave(const char *name);

/**
 * Destroys the shared-memory with given name. Unmaps the pages for all joined processes but
 * NOT for the owner
 *
 * @param name the name
 * @return 0 on success
 */
s32 shm_destroy(const char *name);

/**
 * Removes the process from all shared-memory stuff
 *
 * @param p the process
 */
void shm_remProc(sProc *p);


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

/**
 * Prints all shared-memory regions
 */
void shm_dbg_print(void);

#endif

#endif /* SHAREDMEM_H_ */
