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
#include <esc/mem.h>
#include <esc/time.h>
#include <stdio.h>

#include "mmap.h"

#define MMAP_COUNT		1000

int mod_mmap(A_UNUSED int argc,A_UNUSED char *argv[]) {
	uint64_t start,end;
	size_t i, sizes[] = {0x1000,0x2000,0x4000,0x8000,0x10000,0x20000};
	int j;
	for(i = 0; i < ARRAY_SIZE(sizes); ++i) {
		uint64_t mmapTotal = 0, munmapTotal = 0;
		for(j = 0; j < MMAP_COUNT; ++j) {
			start = rdtsc();
			void *addr = mmap(NULL,sizes[i],0,MAP_PRIVATE,PROT_READ | PROT_WRITE,-1,0);
			end = rdtsc();
			mmapTotal += end - start;
			if(!addr) {
				printe("mmap failed");
				return 1;
			}
			start = rdtsc();
			if(munmap(addr) != 0) {
				printe("munmap failed");
				return 1;
			}
			end = rdtsc();
			munmapTotal += end - start;
		}
		printf("mmap(%zu)  : %Lu cycles/call\n",sizes[i],mmapTotal / MMAP_COUNT);
		printf("munmap(%zu): %Lu cycles/call\n",sizes[i],munmapTotal / MMAP_COUNT);
	}
	return 0;
}
