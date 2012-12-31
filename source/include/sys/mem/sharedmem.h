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
#include <sys/task/proc.h>

#define MAX_SHAREDMEM_NAME	15

/**
 * Inits the shared-memory-module
 */
void shm_init(void);

/**
 * Creates a shared-memory with given name and page-count
 *
 * @param pid the process-id
 * @param name the name
 * @param pageCount the number of pages
 * @return the start-page on success
 */
ssize_t shm_create(pid_t pid,const char *name,size_t pageCount);

/**
 * Joins the shared-memory with given name
 *
 * @param pid the process-id
 * @param name the name
 * @return the start-page on success
 */
ssize_t shm_join(pid_t pid,const char *name);

/**
 * Leaves the shared-memory with given name.
 *
 * @param pid the process-id
 * @param name the name
 * @return 0 on success
 */
int shm_leave(pid_t pid,const char *name);

/**
 * Destroys the shared-memory with given name. Unmaps the pages for all joined processes and
 * the owner.
 *
 * @param pid the process-id
 * @param name the name
 * @return 0 on success
 */
int shm_destroy(pid_t pid,const char *name);

/**
 * Joins all shared-memory areas with <child> that have been created by <parent> or which <parent>
 * has joined. Assumes that the regions have already been cloned.
 *
 * @param parent the parent-process-id
 * @param child the child-process-id
 * @return 0 on success
 */
int shm_cloneProc(pid_t parent,pid_t child);

/**
 * Removes the process from all shared-memory stuff
 *
 * @param pid the process-id
 */
void shm_remProc(pid_t pid);

/**
 * Prints all shared-memory regions
 */
void shm_print(void);
