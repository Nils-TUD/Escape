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
#include <esc/time.h>
#include <esc/io.h>
#include <stdio.h>

#include "../modules.h"

#define GLOBAL_IDENT	0x88212723
#define LOCAL_IDENT		0x88212724
#define TEST_COUNT		10000

typedef int (*fLock)(ulong ident,uint flags);
typedef int (*fUnlock)(ulong ident);

static int sem_lock(ulong ident,A_UNUSED uint flags) {
	return semdown(ident);
}
static int sem_unlock(ulong ident) {
	return semup(ident);
}
static int gsem_lock(ulong ident,A_UNUSED uint flags) {
	return gsemdown(ident);
}
static int gsem_unlock(ulong ident) {
	return gsemup(ident);
}

static void run_test(ulong ident,fLock lockFunc,fUnlock unlockFunc) {
	uint64_t start,end;
	uint64_t lockTotal = 0,unlockTotal = 0;
	for(int i = 0; i < TEST_COUNT; ++i) {
		start = rdtsc();
		lockFunc(ident,LOCK_EXCLUSIVE);
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

int mod_locks(A_UNUSED int argc,A_UNUSED char *argv[]) {
	printf("Local locks...\n");
	fflush(stdout);
	run_test(LOCAL_IDENT,lock,unlock);

	printf("Global locks...\n");
	fflush(stdout);
	run_test(GLOBAL_IDENT,lockg,unlockg);

	printf("Local Semaphores...\n");
	fflush(stdout);
	int sem = semcreate(1);
	if(sem < 0) {
		printe("Unable to get sem");
		return 1;
	}
	run_test(sem,sem_lock,sem_unlock);
	semdestroy(sem);

	printf("Global Semaphores...\n");
	fflush(stdout);
	int gsem = gsemcreate("testperf",0600);
	if(gsem < 0) {
		printe("Unable to get sem");
		return 1;
	}
	run_test(gsem,gsem_lock,gsem_unlock);
	gsemclose(gsem);
	return 0;
}
