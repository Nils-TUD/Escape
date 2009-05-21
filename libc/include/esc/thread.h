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

#ifndef THREAD_H_
#define THREAD_H_

#include <esc/common.h>

/* the thread-entry-point-function */
typedef int (*fThreadEntry)(void);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @return the id of the current thread
 */
tTid gettid(void);

/**
 * @return the number of threads in the current process
 */
u32 getThreadCount(void);

/**
 * Starts a new thread
 *
 * @param entryPoint the entry-point of the thread
 * @return new tid for current thread, 0 for new thread, < 0 if failed
 */
s32 startThread(fThreadEntry entryPoint);

#ifdef __cplusplus
}
#endif

#endif /* THREAD_H_ */
