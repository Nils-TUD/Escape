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

/**
 * Aquires the given mutex. It won't use busy-waiting here, but suspend the thread when the mutex
 * is not available.
 *
 * @param m the mutex
 */
void mutex_acquire(mutex_t *m);

/**
 * Tries to acquire the given mutex. If its locked, it does not block, but return false.
 *
 * @param m the mutex
 * @return true if the mutex has been acquired
 */
bool mutex_tryAquire(mutex_t *m);

/**
 * Releases the given mutex
 *
 * @param m the mutex
 */
void mutex_release(mutex_t *m);
