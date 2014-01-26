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
#include <esc/mem.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "../modules.h"

#define TEST_COUNT		2000

typedef ssize_t (*test_func)(int fd,void *buf,size_t count);

static size_t sizes[] = {0x1000,0x2000,0x4000,0x8000,0x10000,0x20000};
static char buffer[0x20000];

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

static int _sharebuf(int dev,size_t size,void **mem,ulong *name,int flags) {
	/* create shm file */
	*mem = NULL;
	int fd = pshm_create(IO_READ | IO_WRITE,0666,name);
	if(fd < 0)
		return fd;

	/* mmap it */
	void *addr = mmap(NULL,size,0,PROT_READ | PROT_WRITE,MAP_SHARED | flags,fd,0);
	if(!addr) {
		int res = -errno;
		pshm_unlink(*name);
		close(fd);
		return res;
	}

	/* share it with device; if it doesn't work, we don't care here */
	int res = 0;//sharefile(dev,addr);
	*mem = addr;
	close(fd);
	return res;
}

static void test_sharebuf(const char *path,size_t bufsize) {
	uint64_t start,end,sharetime,destrtime;

	sharetime = 0;
	destrtime = 0;
	for(int i = 0; i < TEST_COUNT; ++i) {
		int fd = open(path,IO_READ);
		if(fd < 0) {
			printe("open failed");
			return;
		}

		ulong shname;
		void *shmem;
		start = rdtsc();
		if(_sharebuf(fd,bufsize,&shmem,&shname,0) < 0)
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
	int fd = create("/system/test",IO_WRITE,0600);
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
	test_readwrite("/system/test","read",IO_READ,read,false);
	fflush(stdout);
	test_readwrite("/system/test","write",IO_WRITE,(test_func)write,false);
	fflush(stdout);

	printf("Using /home/hrniels/testdir/bbc.bmp...\n");
	test_openseekclose("/home/hrniels/testdir/bbc.bmp");
	fflush(stdout);
	test_readwrite("/home/hrniels/testdir/bbc.bmp","read",IO_READ,read,false);
	fflush(stdout);
	test_readwrite("/home/hrniels/testdir/bbc.bmp","read",IO_READ,read,true);
	fflush(stdout);

	printf("Testing sharebuf and destroybuf...\n");
	test_sharebuf("/home/hrniels/testdir/bbc.bmp",sysconf(CONF_PAGE_SIZE));
	test_sharebuf("/home/hrniels/testdir/bbc.bmp",sysconf(CONF_PAGE_SIZE) * 4);
	test_sharebuf("/home/hrniels/testdir/bbc.bmp",sysconf(CONF_PAGE_SIZE) * 16);
	fflush(stdout);

	if(unlink("/system/test") < 0)
		printe("Unable to unlink test-file");
	return 0;
}
