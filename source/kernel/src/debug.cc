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

#include <sys/common.h>
#include <esc/debug.h>
#include <esc/thread.h>
#include <sys/video.h>
#include <sys/cpu.h>
#include <string.h>
#include <stdarg.h>

static uint64_t start = 0;

void dbg_startTimer(void) {
	start = CPU::rdtsc();
}

void dbg_stopTimer(const char *prefix) {
	uLongLong diff;
	diff.val64 = CPU::rdtsc() - start;
	vid_printf("%s: 0x%08x%08x\n",prefix,diff.val32.upper,diff.val32.lower);
}
