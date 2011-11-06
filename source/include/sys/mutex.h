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

#ifndef MUTEX_H_
#define MUTEX_H_

#include <sys/common.h>
#include <sys/task/thread.h>

/**
 * Aquires the given mutex. It won't use busy-waiting here, but suspend the thread when the mutex
 * is not available.
 *
 * @param t the current thread
 * @param m the mutex
 */
void mutex_aquire(sThread *t,mutex_t *m);

/**
 * Tries to aquire the given mutex. If its locked, it does not block, but return false.
 *
 * @param t the current thread
 * @param m the mutex
 * @return true if the mutex has been aquired
 */
bool mutex_tryAquire(sThread *t,mutex_t *m);

/**
 * Releases the given mutex
 *
 * @param t the current thread
 * @param m the mutex
 */
void mutex_release(sThread *t,mutex_t *m);

#endif /* MUTEX_H_ */
