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

#ifndef MEM_H_
#define MEM_H_

#include <esc/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Changes the size of the process's data-area. If <count> is positive <count> pages
 * will be added to the end of the data-area. Otherwise <count> pages will be removed at the
 * end.
 *
 * @param count the number of pages to add / remove
 * @return the *old* end of the data-segment. A negative value indicates an error
 */
s32 changeSize(s32 count) A_CHECKRET;

/**
 * Maps <count> bytes at <phys> into the virtual user-space and returns the start-address.
 *
 * @param phys the physical start-address to map
 * @param count the number of bytes to map
 * @return the start-address or NULL if an error occurred
 */
void *mapPhysical(u32 phys,u32 count) A_CHECKRET;

/**
 * Creates a shared-memory region
 *
 * @param name the name
 * @param byteCount the number of bytes
 * @return the address on success or NULL
 */
void *createSharedMem(const char *name,u32 byteCount) A_CHECKRET;

/**
 * Joines a shared-memory region
 *
 * @param name the name
 * @return the address on success or NULL
 */
void *joinSharedMem(const char *name) A_CHECKRET;

/**
 * Leaves a shared-memory region
 *
 * @param name the name
 * @return 0 on success
 */
s32 leaveSharedMem(const char *name);

/**
 * Destroys a shared-memory region
 *
 * @param name the name
 * @return 0 on success
 */
s32 destroySharedMem(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* MEM_H_ */
