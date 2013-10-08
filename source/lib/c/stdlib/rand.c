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

#include <esc/common.h>
#include <esc/thread.h>
#include <assert.h>
#include <stdlib.h>

/* source: http://en.wikipedia.org/wiki/Linear_congruential_generator */
static tULock randLock;
static uint randa = 1103515245;
static uint randc = 12345;
static uint lastRand = 0;

int rand(void) {
	int res;
	locku(&randLock);
	lastRand = randa * lastRand + randc;
	res = (int)((uint)(lastRand / 65536) % 32768);
	unlocku(&randLock);
	return res;
}

void srand(uint seed) {
	/* TODO make use of atomic instructions instead of locku */
	assert(crtlocku(&randLock) >= 0);
	lastRand = seed;
}
