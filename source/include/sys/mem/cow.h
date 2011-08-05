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

#ifndef COW_H_
#define COW_H_

#include <sys/common.h>
#include <sys/task/proc.h>

/**
 * Initializes copy-on-write
 */
void cow_init(void);

/**
 * Handles a pagefault for given address. Assumes that the pagefault was caused by a write access
 * to a copy-on-write page!
 *
 * @param pid the process-id of the current process (locked)
 * @param address the address
 * @param frameNumber the frame for that address
 * @return the number of frames that should be added to the current process
 */
size_t cow_pagefault(pid_t pid,uintptr_t address,frameno_t frameNumber);

/**
 * Adds the given frame and process to the cow-list.
 *
 * @param pid the process-id
 * @param frameNo the frame-number
 * @return true if successfull
 */
bool cow_add(pid_t pid,frameno_t frameNo);

/**
 * Removes the given process and frame from the cow-list
 *
 * @param pid the process-id
 * @param frameNo the frame-number
 * @param foundOther will be set to true if another process still uses the frame
 * @return the number of frames to remove from <p>
 */
size_t cow_remove(pid_t pid,frameno_t frameNo,bool *foundOther);

/**
 * Note that this is intended for debugging or similar only! (not very efficient)
 *
 * @return the number of different frames that are in the cow-list
 */
size_t cow_getFrmCount(void);

/**
 * Prints the cow-list
 */
void cow_print(void);

#endif /* COW_H_ */
