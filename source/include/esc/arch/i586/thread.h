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

#include <esc/common.h>
#include <esc/thread.h>

static inline void locku(tULock *l) {
	/* 0 means free, < 0 means taken. we have this slightly odd meaning here to prevent that we
	 * have to initialize all locks with 1 (which would also be arch-dependent here) */
    if(__sync_fetch_and_add(l, -1) <= -1)
    	lock((uint)l,LOCK_EXCLUSIVE);
}

static inline void unlocku(tULock *l) {
    if(__sync_fetch_and_add(l, +1) < -1)
    	unlock((uint)l);
}
