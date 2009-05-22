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
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <esc/signals.h>

static int myThread(void);

u64 pre;
u64 post;

int main(void) {
	startThread(myThread);
	pre = cpu_rdtsc();
	yield();
	yield();
	yield();
	yield();
	u64 diff = post - pre;
	u32 *ptr = (u32*)&diff;
	debugf("diff=0x%x%x\n",*(ptr + 1),*ptr);
	return 0;
}

static int myThread(void) {
	post = cpu_rdtsc();
	return 0;
}
