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

#include <sys/common.h>
#include <sys/arch.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "../modules.h"

#define TEST_COUNT		2000
#define MAP_SIZE		32

static void causePagefaults(const char *path) {
	uint64_t start,end;
	uint64_t total = 0;
	uint64_t min = ULLONG_MAX, max = 0;
	int fd = -1;
	if(path) {
		fd = open(path,O_RDONLY);
		if(fd < 0) {
			printe("Unable to open '%s'",path);
			return;
		}
	}

	for(int j = 0; j < TEST_COUNT; ++j) {
		volatile char *addr = mmap(NULL,MAP_SIZE * PAGE_SIZE,path ? MAP_SIZE * PAGE_SIZE : 0,
			PROT_READ | PROT_WRITE,MAP_PRIVATE,fd,0);
		if(!addr) {
			printe("mmap failed");
			return;
		}

		for(size_t i = 0; i < MAP_SIZE; ++i) {
			start = rdtsc();
			*(addr + i * PAGE_SIZE) = 0;
			end = rdtsc();
			uint64_t duration = end - start;
			if(duration < min)
				min = duration;
			if(duration > max)
				max = duration;
			total += duration;
		}

		if(munmap((void*)addr) != 0)
			printe("munmap failed");
	}
	if(path)
		close(fd);

	printf("%-30s: %Lu cycles average\n",path ? path : "NULL",total / (TEST_COUNT * MAP_SIZE));
	printf("%-30s: %Lu cycles minimum\n",path ? path : "NULL",min);
	printf("%-30s: %Lu cycles maximum\n",path ? path : "NULL",max);
}

int mod_pagefault(A_UNUSED int argc,A_UNUSED char *argv[]) {
	size_t i;
	size_t total = MAP_SIZE * PAGE_SIZE;
	char *buffer = malloc(total);
	if(!buffer) {
		printe("Unable to create buffer");
		return 1;
	}
	int fd = create("/sys/test",O_WRONLY,0600);
	if(fd < 0) {
		printe("open of /sys/test failed");
		return 1;
	}
	srand(time(NULL));
	for(i = 0; i < total; ++i)
		buffer[i] = rand();
	if(write(fd,buffer,total) != (ssize_t)total) {
		printe("write failed");
		return 1;
	}
	close(fd);

	causePagefaults(NULL);
	causePagefaults("/sys/test");
	causePagefaults("/home/hrniels/testdir/bbc.bmp");

	if(unlink("/sys/test") < 0)
		printe("Unable to unlink test-file");
	free(buffer);
	return 0;
}
