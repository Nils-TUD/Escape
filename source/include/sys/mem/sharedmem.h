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
ssize_t shm_create(sProc *p,const char *name,size_t pageCount);

/**
 * Joins the shared-memory with given name
 *
 * @param p the process
 * @param name the name
 * @return the start-page on success
 */
ssize_t shm_join(sProc *p,const char *name);

/**
 * Leaves the shared-memory with given name.
 *
 * @param p the process
 * @param name the name
 * @return 0 on success
 */
int shm_leave(sProc *p,const char *name);

/**
 * Destroys the shared-memory with given name. Unmaps the pages for all joined processes and
 * the owner.
 *
 * @param p the process
 * @param name the name
 * @return 0 on success
 */
int shm_destroy(sProc *p,const char *name);

/**
 * Joins all shared-memory areas with <child> that have been created by <parent> or which <parent>
 * has joined. Assumes that the regions have already been cloned.
 *
 * @param parent the parent-process
 * @param child the child-process
 * @return 0 on success
 */
int shm_cloneProc(const sProc *parent,sProc *child);

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
