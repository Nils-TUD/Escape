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
#include <proc.h>

/**
 * Creates a shared-memory with given name and page-count
 *
 * @param name the name
 * @param pageCount the number of pages
 * @return the start-page on success
 */
s32 shm_create(char *name,u32 pageCount);

/**
 * Joins the shared-memory with given name. Will copy the pages into the end of the data-segment
 *
 * @param name the name
 * @return the start-page on success
 */
s32 shm_join(char *name);

/**
 * Leaves the shared-memory with given name. Will NOT unmap the pages!
 *
 * @param name the name
 * @return 0 on success
 */
s32 shm_leave(char *name);

/**
 * Destroys the shared-memory with given name. Unmaps the pages for all member-processes
 *
 * @param name the name
 * @return 0 on success
 */
s32 shm_destroy(char *name);

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
