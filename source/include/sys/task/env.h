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

#ifndef ENV_H_
#define ENV_H_

#include <sys/common.h>
#include <sys/task/proc.h>

/**
 * Copies the env-variable-name with given index to <dst>
 *
 * @param p the process
 * @param index the index
 * @param dst the destination-string
 * @param size the size of dst
 * @return true if existing and successfully copied
 */
bool env_geti(sProc *p,size_t index,char *dst,size_t size);

/**
 * Copies the env-variable-value with given name to <dst>
 *
 * @param p the process
 * @param name the variable-name
 * @param dst the destination-string
 * @param size the size of dst
 * @return true if existing and successfully copied
 */
bool env_get(sProc *p,const char *name,char *dst,size_t size);

/**
 * Sets <name> to <value>
 *
 * @param p the process
 * @param name the variable-name
 * @param value the (new) value
 * @return true if successfull
 */
bool env_set(sProc *p,const char *name,const char *value);

/**
 * Removes all env-variables for given process
 *
 * @param p the process
 */
void env_removeFor(sProc *p);

/**
 * Prints all env-vars of given process
 *
 * @param p the process
 */
void env_printAllOf(sProc *p);

#endif /* ENV_H_ */
