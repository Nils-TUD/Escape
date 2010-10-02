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

#include <sys/common.h>
#include <sys/task/proc.h>

/**
 * Inits the shared-memory-module
 */
void shm_init(void);

/**
 * Creates a shared-memory with given name and page-count
 *
 * @param p the process
 * @param name the name
 * @param pageCount the number of pages
 * @return the start-page on success
 */
s32 shm_create(sProc *p,const char *name,u32 pageCount);

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
u32 shm_getAddrOfOther(const sProc *p,u32 addr,const sProc *other);

/**
 * Joins the shared-memory with given name
 *
 * @param p the process
 * @param name the name
 * @return the start-page on success
 */
s32 shm_join(sProc *p,const char *name);

/**
 * Leaves the shared-memory with given name.
 *
 * @param p the process
 * @param name the name
 * @return 0 on success
 */
s32 shm_leave(sProc *p,const char *name);

/**
 * Destroys the shared-memory with given name. Unmaps the pages for all joined processes and
 * the owner.
 *
 * @param p the process
 * @param name the name
 * @return 0 on success
 */
s32 shm_destroy(sProc *p,const char *name);

/**
 * Joins all shared-memory areas with <child> that have been created by <parent> or which <parent>
 * has joined. Assumes that the regions have already been cloned.
 *
 * @param parent the parent-process
 * @param child the child-process
 * @return 0 on success
 */
s32 shm_cloneProc(const sProc *parent,sProc *child);

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
