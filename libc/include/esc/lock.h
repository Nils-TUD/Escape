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

#ifndef LOCK_H_
#define LOCK_H_

#include "common.h"

typedef u32 tULock;

/**
 * Aquires the process-local lock with ident. If it exists and is locked, the process waits until
 * it's unlocked
 *
 * @param ident to identify the lock
 * @return 0 on success
 */
s32 lock(u32 ident);

/**
 * Aquires a process-local lock in user-space. If the given lock is in use, the process waits
 * (actively) until the lock is unused.
 *
 * @param lock the lock
 */
void locku(tULock *lock);

/**
 * Aquires a global lock with given ident. If it exists and is locked, the process waits until
 * it's unlocked
 *
 * @param ident to identify the lock
 * @return 0 on success
 */
s32 lockg(u32 ident);

/**
 * Releases the process-local lock with given ident
 *
 * @param ident to identify the lock
 * @return 0 on success
 */
s32 unlock(u32 ident);

/**
 * Releases the process-local lock that is locked in user-space
 *
 * @param lock the lock
 */
void unlocku(tULock *lock);

/**
 * Releases the global lock with given ident
 *
 * @param ident to identify the lock
 * @return 0 on success
 */
s32 unlockg(u32 ident);

#endif /* LOCK_H_ */
