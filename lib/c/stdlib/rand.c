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

#include <esc/common.h>
#include <esc/thread.h>
#include <stdlib.h>

/* source: http://en.wikipedia.org/wiki/Linear_congruential_generator */
static tULock randLock = 0;
static u32 randa = 1103515245;
static u32 randc = 12345;
static u32 lastRand = 0;

s32 rand(void) {
	s32 res;
	locku(&randLock);
	lastRand = randa * lastRand + randc;
	res = (s32)((u32)(lastRand / 65536) % 32768);
	unlocku(&randLock);
	return res;
}

void srand(u32 seed) {
	lastRand = seed;
}
