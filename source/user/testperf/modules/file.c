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
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "../modules.h"

#define TEST_COUNT		2000

typedef ssize_t (*test_func)(int fd,void *buf,size_t count);

static size_t sizes[] = {0x1000,0x2000,0x4000,0x8000,0x10000,0x20000,0x40000,0x80000,0x100000};
static char buffer[0x100000];

static void test_openseekclose(const char *path) {
	uint64_t openTotal = 0, closeTotal = 0, seekTotal = 0;
	uint64_t start,end;
	int j;
	for(j = 0; j < TEST_COUNT; ++j) {
		start = rdtsc();
		int fd = open(path,O_RDONLY);
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

static void test_readwrite(const char *path,const char *name,int flags,test_func func,bool shared) {
	uint64_t start,end;
	uint64_t times[ARRAY_SIZE(sizes)] = {0};
	int fd = open(path,flags);
	if(fd < 0) {
		printe("open failed");
		return;
	}

	ulong shname;
	void *shmem;
	if(!shared)
		shmem = mmap(NULL,sizeof(buffer),0,PROT_READ | PROT_WRITE,MAP_PRIVATE,-1,0);
	else if(sharebuf(fd,sizeof(buffer),&shmem,&shname,0) < 0)
		printe("Unable to share memory");
	if(!shmem)
		error("Unable to map shared memory");

	size_t i,j;
	for(i = 0; i < TEST_COUNT; ++i) {
		for(j = 0; j < ARRAY_SIZE(sizes); ++j) {
			start = rdtsc();
			ssize_t res = func(fd,shmem,sizes[j]);
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

	if(shared)
		destroybuf(shmem,shname);
	close(fd);
	for(j = 0; j < ARRAY_SIZE(sizes); ++j) {
		printf("%s(%3zuK): %6Lu cycles/call, %Lu MB/s (%s)\n",
				name,sizes[j] / 1024,times[j] / TEST_COUNT,
				(sizes[j] * TEST_COUNT) / tsctotime(times[j]),
				shared ? "shared" : "non-shared");
	}
}

static void test_stat(const char *path) {
	uint64_t start,end,total = 0;
	struct stat info;
	for(int i = 0; i < TEST_COUNT; ++i) {
		start = rdtsc();
		int res = stat(path,&info);
		end = rdtsc();
		if(res < 0)
			printe("stat of '%s' failed",path);
		total += end - start;
	}

	printf("stat      : %6Lu cycles/call\n",total / TEST_COUNT);
}

static void test_fstat(const char *path) {
	uint64_t start,end,total = 0;
	struct stat info;
	for(int i = 0; i < TEST_COUNT; ++i) {
		start = rdtsc();
		int fd = open(path,O_RDONLY);
		if(fd >= 0) {
			if(fstat(fd,&info) < 0)
				printe("fstat of '%s' failed",path);
			close(fd);
		}
		else
			printe("open of '%s' failed",path);
		end = rdtsc();
		total += end - start;
	}

	printf("fstat     : %6Lu cycles/call\n",total / TEST_COUNT);
}

static void test_sharebuf(const char *path,size_t bufsize) {
	uint64_t start,end,sharetime,destrtime;

	sharetime = 0;
	destrtime = 0;
	for(int i = 0; i < TEST_COUNT; ++i) {
		int fd = open(path,O_RDONLY);
		if(fd < 0) {
			printe("open failed");
			return;
		}

		ulong shname;
		void *shmem;
		start = rdtsc();
		if(sharebuf(fd,bufsize,&shmem,&shname,0) < 0)
			printe("Unable to share memory");
		end = rdtsc();
		if(!shmem)
			error("Unable to map shared memory");
		sharetime += end - start;

		start = rdtsc();
		destroybuf(shmem,shname);
		end = rdtsc();
		destrtime += end - start;

		close(fd);
	}

	printf("sharebuf  (%6zub): %Lu cycles/call\n",bufsize,sharetime / TEST_COUNT);
	printf("destroybuf(%6zub): %Lu cycles/call\n",bufsize,destrtime / TEST_COUNT);
}

int mod_file(A_UNUSED int argc,A_UNUSED char *argv[]) {
	size_t i;
	int fd = create("/sys/test",O_WRONLY,0600);
	if(fd < 0) {
		printe("open of /sys/test failed");
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

	printf("Using /sys/test...\n");
	test_openseekclose("/sys/test");
	fflush(stdout);
	test_readwrite("/sys/test","read",O_READ,read,false);
	fflush(stdout);
	test_readwrite("/sys/test","write",O_WRONLY,(test_func)write,false);
	fflush(stdout);
	test_stat("/sys/test");
	fflush(stdout);
	test_fstat("/sys/test");
	fflush(stdout);

	const char *filename = "/zeros";
	printf("\nUsing %s...\n",filename);
	test_openseekclose(filename);
	fflush(stdout);
	test_readwrite(filename,"read",O_READ,read,false);
	fflush(stdout);
	test_readwrite(filename,"read",O_READ,read,true);
	fflush(stdout);
	test_stat(filename);
	fflush(stdout);
	test_fstat(filename);
	fflush(stdout);

	printf("\nTesting sharebuf and destroybuf...\n");
	test_sharebuf(filename,PAGE_SIZE);
	test_sharebuf(filename,PAGE_SIZE * 4);
	test_sharebuf(filename,PAGE_SIZE * 16);
	fflush(stdout);

	if(unlink("/sys/test") < 0)
		printe("Unable to unlink test-file");
	return 0;
}
