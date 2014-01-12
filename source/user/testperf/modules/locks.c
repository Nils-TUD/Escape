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
#include <esc/thread.h>
#include <esc/sync.h>
#include <esc/time.h>
#include <esc/io.h>
#include <stdio.h>

#include "../modules.h"

#define TEST_COUNT		100000

typedef int (*fLock)(int id);
typedef int (*fUnlock)(int id);

static void run_test(ulong ident,fLock lockFunc,fUnlock unlockFunc) {
	uint64_t start,end;
	uint64_t lockTotal = 0,unlockTotal = 0;
	for(int i = 0; i < TEST_COUNT; ++i) {
		start = rdtsc();
		lockFunc(ident);
		end = rdtsc();
		lockTotal += end - start;

		start = rdtsc();
		unlockFunc(ident);
		end = rdtsc();
		unlockTotal += end - start;
	}

	printf("  lock(): %Lu cycles/call\n",lockTotal / TEST_COUNT);
	printf("unlock(): %Lu cycles/call\n",unlockTotal / TEST_COUNT);
}

static int sem1;
static int sem2;

static int thread_pingpong(A_UNUSED void *arg) {
	uint64_t start,end;
	int s1 = arg ? sem1 : sem2;
	int s2 = arg ? sem2 : sem1;
	start = rdtsc();
	for(int i = 0; i < TEST_COUNT; ++i) {
		semdown(s1);
		semup(s2);
	}
	end = rdtsc();
	semdown(sem1);
	printf("[%3d] %Lu cycles/pingpong\n",gettid(),(end - start) / TEST_COUNT);
	semup(sem1);
	return 0;
}

int mod_locks(A_UNUSED int argc,A_UNUSED char *argv[]) {
	printf("Local Semaphores...\n");
	fflush(stdout);
	int sem = semcrt(1);
	if(sem < 0) {
		printe("Unable to get sem");
		return 1;
	}
	run_test(sem,semdown,semup);
	semdestr(sem);

	printf("Global Semaphores...\n");
	fflush(stdout);
	int gsem = open("/system",IO_READ);
	if(gsem < 0) {
		printe("Unable to get sem");
		return 1;
	}
	run_test(gsem,fsemdown,fsemup);
	close(gsem);

	printf("Semaphore pingpong...\n");
	fflush(stdout);
	sem1 = semcrt(1);
	sem2 = semcrt(0);
	if(sem1 < 0 || sem2 < 0) {
		printe("Unable to create sems");
		return 1;
	}
	if(startthread(thread_pingpong,(void*)0) < 0 || startthread(thread_pingpong,(void*)1) < 0) {
		printe("Unable to start thread");
		return 1;
	}
	join(0);
	semdestr(sem2);
	semdestr(sem1);
	return 0;
}
