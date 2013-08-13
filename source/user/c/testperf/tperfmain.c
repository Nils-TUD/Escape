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
#include <esc/proc.h>
#include <esc/mem.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>

#define SYSC_COUNT		100000
#define MMAP_COUNT		1000
#define FILE_COUNT		1000

static void test_fileread(const char *path);
static void test_getpidsysc(void);
static void test_mapunmap(void);
static void test_file(void);

static char buffer[0x10000];

int main(void) {
	test_getpidsysc();
	test_mapunmap();
	test_file();
	return EXIT_SUCCESS;
}

static void test_getpidsysc(void) {
	uint64_t start = rdtsc();
	int i;
	for(i = 0; i < SYSC_COUNT; ++i)
		getpid();
	uint64_t end = rdtsc();
	printf("getpid(): %Lu cycles/call\n",(end - start) / SYSC_COUNT);
}

static void test_mapunmap(void) {
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
			if(!addr)
				error("mmap failed");
			start = rdtsc();
			if(munmap(addr) != 0)
				error("munmap failed");
			end = rdtsc();
			munmapTotal += end - start;
		}
		printf("mmap(%zu)  : %Lu cycles/call\n",sizes[i],mmapTotal / MMAP_COUNT);
		printf("munmap(%zu): %Lu cycles/call\n",sizes[i],munmapTotal / MMAP_COUNT);
	}
}

static void test_file(void) {
	size_t i;
	int fd = open("/system/test",IO_CREATE | IO_WRITE);
	if(fd < 0)
		error("open of /system/test failed");
	srand(time(NULL));
	for(i = 0; i < sizeof(buffer); ++i)
		buffer[i] = rand();
	if(write(fd,buffer,sizeof(buffer)) != sizeof(buffer))
		error("write failed");
	close(fd);

	test_fileread("/system/test");
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
		if(fd < 0)
			error("open failed");

		for(i = 0; i < ARRAY_SIZE(sizes); ++i) {
			assert(sizes[i] <= sizeof(buffer));
			start = rdtsc();
			if(read(fd,buffer,sizes[i]) != (ssize_t)sizes[i])
				error("read failed");
			end = rdtsc();
			reads[i] += end - start;
			start = rdtsc();
			if(seek(fd,0,SEEK_SET) < 0)
				error("seek failed");
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
