/**
 * $Id: cpu.c 870 2011-05-29 10:46:59Z nasmussen $
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

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/cpu.h>
#include <sys/printf.h>
#include <sys/video.h>
#include <string.h>

uint64_t cpu_rdtsc(void) {
	/* TODO not implemented yet */
	return 0;
}

void cpu_sprintf(sStringBuffer *buf) {
	size_t size;
	prf_sprintf(buf,"%-12s%s\n","Vendor:","THM");
	prf_sprintf(buf,"%-12s%s\n","Model:","ECO32");
}
