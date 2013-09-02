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
#include <esc/time.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "fileread.h"

#define FILE_COUNT		1000

static char buffer[0x10000];

static void test_fileread(const char *path);

int mod_fileread(A_UNUSED int argc,A_UNUSED char *argv[]) {
	size_t i;
	int fd = open("/system/test",IO_CREATE | IO_WRITE);
	if(fd < 0) {
		printe("open of /system/test failed");
		return 1;
	}
	srand(time(NULL));
	for(i = 0; i < sizeof(buffer); ++i)
		buffer[i] = rand();
	if(write(fd,buffer,sizeof(buffer)) != sizeof(buffer)) {
		printe("write failed");
		return 1;
	}
	close(fd);

	test_fileread("/system/test");
	return 0;
}

static void test_fileread(const char *path) {
	size_t i, sizes[] = {0x1000,0x2000,0x4000,0x8000,0x10000};
	uint64_t start,end;
	uint64_t openTotal = 0, closeTotal = 0, reads[5] = {0}, seeks[5] = {0};
	int j;
	for(j = 0; j < FILE_COUNT; ++j) {
		start = rdtsc();
		int fd = open(path,IO_READ);
		end = rdtsc();
		openTotal += end - start;
		if(fd < 0) {
			printe("open failed");
			return;
		}

		for(i = 0; i < ARRAY_SIZE(sizes); ++i) {
			assert(sizes[i] <= sizeof(buffer));
			start = rdtsc();
			if(read(fd,buffer,sizes[i]) != (ssize_t)sizes[i]) {
				printe("read failed");
				return;
			}
			end = rdtsc();
			reads[i] += end - start;
			start = rdtsc();
			if(seek(fd,0,SEEK_SET) < 0) {
				printe("seek failed");
				return;
			}
			end = rdtsc();
			seeks[i] += end - start;
		}

		start = rdtsc();
		close(fd);
		end = rdtsc();
		closeTotal += end - start;
	}

	printf("open         : %Lu cycles/call\n",openTotal / FILE_COUNT);
	printf("close        : %Lu cycles/call\n",closeTotal / FILE_COUNT);
	for(i = 0; i < ARRAY_SIZE(sizes); ++i) {
		printf("read(%5zu)  : %Lu cycles/call\n",sizes[i],reads[i] / FILE_COUNT);
		printf("seek(%5zu)  : %Lu cycles/call\n",sizes[i],seeks[i] / FILE_COUNT);
	}
}
