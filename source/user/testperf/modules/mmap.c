/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <sys/mman.h>
#include <esc/time.h>
#include <stdio.h>
#include <stdlib.h>

#include "../modules.h"

#define MMAP1_COUNT		1000
#define MMAP2_COUNT		100

static size_t sizes[] = {0x1000,0x2000,0x4000,0x8000,0x10000,0x20000};

static void mmapunmap(int flags,int fd) {
	uint64_t start,end;
	size_t i;
	for(i = 0; i < ARRAY_SIZE(sizes); ++i) {
		uint64_t mmapTotal = 0, munmapTotal = 0;
		int j;
		for(j = 0; j < MMAP1_COUNT; ++j) {
			start = rdtsc();
			void *addr = mmap(NULL,sizes[i],0,PROT_READ | PROT_WRITE,flags,fd,0);
			end = rdtsc();
			mmapTotal += end - start;
			if(!addr) {
				printe("mmap failed");
				return;
			}
			start = rdtsc();
			if(munmap(addr) != 0) {
				printe("munmap failed");
				return;
			}
			end = rdtsc();
			munmapTotal += end - start;
		}
		printf("mmap(%3zuK)  : %Lu cycles/call\n",sizes[i] / 1024,mmapTotal / MMAP1_COUNT);
		printf("munmap(%3zuK): %Lu cycles/call\n",sizes[i] / 1024,munmapTotal / MMAP1_COUNT);
	}
}

static void multimmap(void) {
	static void *addrs[MMAP2_COUNT];
	uint64_t start,end;
	size_t i;
	for(i = 0; i < ARRAY_SIZE(sizes); ++i) {
		uint64_t mmapTotal = 0, munmapTotal = 0;
		int j;
		for(j = 0; j < MMAP2_COUNT; ++j) {
			start = rdtsc();
			addrs[j] = mmap(NULL,sizes[i],0,PROT_READ | PROT_WRITE,MAP_PRIVATE,-1,0);
			end = rdtsc();
			mmapTotal += end - start;
			if(!addrs[j]) {
				printe("mmap failed");
				return;
			}
		}
		for(j = 0; j < MMAP2_COUNT; ++j) {
			start = rdtsc();
			if(munmap(addrs[j]) != 0) {
				printe("munmap failed");
				return;
			}
			end = rdtsc();
			munmapTotal += end - start;
		}
		printf("mmap(%3zuK)  : %Lu cycles/call\n",sizes[i] / 1024,mmapTotal / MMAP2_COUNT);
		printf("munmap(%3zuK): %Lu cycles/call\n",sizes[i] / 1024,munmapTotal / MMAP2_COUNT);
	}
}

int mod_mmap(A_UNUSED int argc,A_UNUSED char *argv[]) {
	printf("A sequence of mmap's and munmap's...\n");
	mmapunmap(MAP_PRIVATE,-1);
	fflush(stdout);

	printf("A sequence of mmap's, followed by a sequence of munmap's...\n");
	multimmap();
	fflush(stdout);

	{
		const char *path = "/home/hrniels/testdir/bbc.bmp";
		printf("mmap of %s...\n",path);
		int fd = open(path,O_RDONLY);
		if(fd < 0)
			error("Unable to open '%s'",path);
		mmapunmap(MAP_SHARED,fd);
		close(fd);
		fflush(stdout);
	}

	{
		const char *path = "/bin/init";
		printf("mmap of %s...\n",path);
		int fd = open(path,O_RDONLY);
		if(fd < 0)
			error("Unable to open '%s'",path);
		mmapunmap(MAP_SHARED,fd);
		close(fd);
		fflush(stdout);
	}
	return 0;
}
