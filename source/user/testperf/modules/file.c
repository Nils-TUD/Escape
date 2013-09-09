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

#include "file.h"

#define TEST_COUNT		1000

typedef ssize_t (*test_func)(int fd,void *buf,size_t count);

static size_t sizes[] = {0x1000,0x2000,0x4000,0x8000,0x10000};
static char buffer[0x10000];

static void test_openseekclose(const char *path) {
	uint64_t openTotal = 0, closeTotal = 0, seekTotal = 0;
	uint64_t start,end;
	int j;
	for(j = 0; j < TEST_COUNT; ++j) {
		start = rdtsc();
		int fd = open(path,IO_READ);
		end = rdtsc();
		openTotal += end - start;
		if(fd < 0) {
			printe("open failed");
			return;
		}

		start = rdtsc();
		off_t res = seek(fd,0,SEEK_SET);
		end = rdtsc();
		seekTotal += end - start;
		if(res < 0) {
			printe("seek failed");
			return;
		}

		start = rdtsc();
		close(fd);
		end = rdtsc();
		closeTotal += end - start;
	}
	printf("open      : %Lu cycles/call\n",openTotal / TEST_COUNT);
	printf("seek      : %Lu cycles/call\n",seekTotal / TEST_COUNT);
	printf("close     : %Lu cycles/call\n",closeTotal / TEST_COUNT);
}

static void test_readwrite(const char *path,const char *name,int flags,test_func func) {
	uint64_t start,end;
	uint64_t times[ARRAY_SIZE(sizes)] = {0};
	int fd = open(path,flags);
	if(fd < 0) {
		printe("open failed");
		return;
	}
	size_t i,j;
	for(i = 0; i < TEST_COUNT; ++i) {
		for(j = 0; j < ARRAY_SIZE(sizes); ++j) {
			assert(sizes[j] <= sizeof(buffer));
			start = rdtsc();
			ssize_t res = func(fd,buffer,sizes[j]);
			end = rdtsc();
			times[j] += end - start;
			if(res != (ssize_t)sizes[j]) {
				printe("read/write failed");
				return;
			}
			if(seek(fd,0,SEEK_SET) < 0) {
				printe("seek failed");
				return;
			}
		}
	}
	close(fd);
	for(j = 0; j < ARRAY_SIZE(sizes); ++j) {
		printf("%s(%3zuK): %6Lu cycles/call, %Lu MB/s\n",
				name,sizes[j] / 1024,times[j] / TEST_COUNT,
				(sizes[j] * TEST_COUNT) / tsctotime(times[j]));
	}
}

int mod_file(A_UNUSED int argc,A_UNUSED char *argv[]) {
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

	printf("Using /system/test...\n");
	test_openseekclose("/system/test");
	fflush(stdout);
	test_readwrite("/system/test","read",IO_READ,read);
	fflush(stdout);
	test_readwrite("/system/test","write",IO_WRITE,(test_func)write);
	fflush(stdout);

	printf("Using /home/hrniels/testdir/bbc.bmp...\n");
	test_openseekclose("/home/hrniels/testdir/bbc.bmp");
	fflush(stdout);
	test_readwrite("/home/hrniels/testdir/bbc.bmp","read",IO_READ,read);
	fflush(stdout);
	return 0;
}
