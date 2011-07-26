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

#ifndef GROUPS_H_
#define GROUPS_H_

#include <sys/common.h>
#include <sys/task/proc.h>

/**
 * Allocates a new group-descriptor with <groups> (<count> groups) and assigns it to the given
 * process.
 *
 * @param p the process
 * @param count the number of groups
 * @param groups the groups-array (will be copied)
 * @return true if successfull
 */
bool groups_set(sProc *p,size_t count,USER const gid_t *groups);

/**
 * Lets process <dst> join the groups of process <src>.
 *
 * @param dst the dest-process
 * @param src the src-process
 */
void groups_join(sProc *dst,const sProc *src);

/**
 * Copies the group-ids from the given process into the given list. If <size> is 0, the number
 * of groups is returned
 *
 * @param p the process
 * @param list the destination list
 * @param size the size of the list
 * @return the number of copied groups
 */
size_t groups_get(const sProc *p,gid_t *list,size_t size);

/**
 * Checks whether the groups of process <p> contain <gid>.
 *
 * @param p the process
 * @param gid the group-id
 * @return true if so
 */
bool groups_contains(const sProc *p,gid_t gid);

/**
 * Lets the given process leave its groups
 *
 * @param p the process
 */
void groups_leave(sProc *p);

/**
 * Prints the groups of the given process
 *
 * @param p the process
 */
void groups_print(const sProc *p);

#endif /* GROUPS_H_ */
