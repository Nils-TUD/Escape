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
 * Returns the env-variable-name with given index
 *
 * @param index the index
 * @return the name of it or NULL if the index does not exist (or it failed for another reason)
 */
char *getEnvByIndex(u32 index);

/**
 * Returns the value of the given environment-variable. Note that you have to copy the value
 * if you want to keep it!
 *
 * @param name the environment-variable-name
 * @return the value or NULL if failed
 */
char *getEnv(const char *name);

/**
 * Sets the environment-variable <name> to <value>.
 *
 * @param name the name
 * @param value the value
 * @return 0 on success
 */
s32 setEnv(const char *name,const char *value);

#endif /* ENV_H_ */
