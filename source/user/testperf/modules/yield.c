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
#include <esc/thread.h>
#include <esc/sync.h>
#include <esc/proc.h>
#include <esc/time.h>
#include <stdio.h>
#include <stdlib.h>

#include "../modules.h"

#define THREAD_COUNT	2
#define SYSC_COUNT		100000

static int sm;

static int thread_func(A_UNUSED void *arg) {
	int i;
	semdown(sm);
	uint64_t start = rdtsc();
	for(i = 0; i < SYSC_COUNT; ++i)
		yield();
	uint64_t end = rdtsc();
	/* actually, we measure 2 yields per step */
	printf("[%4d] %Lu cycles/call\n",gettid(),(end - start) / (SYSC_COUNT * 2));
	return 0;
}

static void intra_yield(void) {
	int i;
	for(i = 0; i < THREAD_COUNT; ++i) {
		if(startthread(thread_func,NULL) < 0)
			printe("startthread failed");
	}
	for(i = 0; i < THREAD_COUNT; ++i)
		semup(sm);
	join(0);
}

static void inter_yield(void) {
	int i;
	for(i = 0; i < THREAD_COUNT; ++i) {
		if(fork() == 0) {
			thread_func(NULL);
			exit(0);
		}
	}
	for(i = 0; i < THREAD_COUNT; ++i)
		semup(sm);
	for(i = 0; i < THREAD_COUNT; ++i)
		waitchild(NULL);
}

int mod_yield(A_UNUSED int argc,A_UNUSED char *argv[]) {
	sm = semcrt(0);
	if(sm < 0)
		error("Unable to create semaphore");
	printf("Intra-process...\n");
	fflush(stdout);
	intra_yield();
	printf("Inter-process...\n");
	fflush(stdout);
	inter_yield();
	semdestr(sm);
	return 0;
}
