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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the env-variable-name with given index
 *
 * @param name the name where to write to
 * @param nameSize the size of the space name points to
 * @param index the index
 * @return true on success
 */
bool getEnvByIndex(char *name,u32 nameSize,u32 index);

/**
 * Returns the value of the given environment-variable
 *
 * @param value the value where to write to
 * @param valSize the size of the space value points to
 * @param name the environment-variable-name
 * @return true on success
 */
bool getEnv(char *value,u32 valSize,const char *name);

/**
 * Sets the environment-variable <name> to <value>.
 *
 * @param name the name
 * @param value the value
 * @return 0 on success
 */
s32 setEnv(const char *name,const char *value);

#ifdef __cplusplus
}
#endif

#endif /* ENV_H_ */
