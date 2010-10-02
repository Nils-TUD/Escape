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

#ifndef ENV_H_
#define ENV_H_

#include <esc/common.h>

/**
 * Returns the env-variable with given index
 *
 * @param pid the process-id
 * @param index the index
 * @return the env-variable-name or NULL if there is none
 */
const char *env_geti(tPid pid,u32 index);

/**
 * Returns the env-variable with given name
 *
 * @param pid the process-id
 * @param name the variable-name
 * @return the variable-value or NULL if not found
 */
const char *env_get(tPid pid,const char *name);

/**
 * Sets <name> to <value>
 *
 * @param pid the process-id
 * @param name the variable-name
 * @param value the (new) value
 * @return true if successfull
 */
bool env_set(tPid pid,const char *name,const char *value);

/**
 * Removes all env-variables for given process
 *
 * @param pid the process-id
 */
void env_removeFor(tPid pid);


#if DEBUGGING

/**
 * Prints all env-vars of given process
 *
 * @param pid the process-id
 */
void env_dbg_printAllOf(tPid pid);

#endif

#endif /* ENV_H_ */
