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
#include <esc/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"

#define THREAD_COUNT	10
#define LOCK_IDENT		0x11111111

static int myThread(A_UNUSED void *arg) {
	size_t i;
	lock(LOCK_IDENT,LOCK_EXCLUSIVE);
	printf("Thread %d starts\n",gettid());
	fflush(stdout);
	unlock(LOCK_IDENT);
	for(i = 0; i < 10; i++)
		sleep(100);
	lock(LOCK_IDENT,LOCK_EXCLUSIVE);
	printf("Thread %d is done\n",gettid());
	fflush(stdout);
	unlock(LOCK_IDENT);
	return 0;
}

int mod_thread(A_UNUSED int argc,A_UNUSED char *argv[]) {
	int threads[THREAD_COUNT];
	size_t i;
	for(i = 0; i < THREAD_COUNT; i++)
		assert((threads[i] = startthread(myThread,NULL)) >= 0);
	sleep(100);
	for(i = 0; i < THREAD_COUNT / 2; i++)
		assert(suspend(threads[i]) == 0);
	sleep(500);
	for(i = 0; i < THREAD_COUNT / 2; i++)
		assert(resume(threads[i]) == 0);
	for(i = 0; i < THREAD_COUNT; i++)
		join(threads[i]);
	assert(getthreadcnt() == 1);
	return EXIT_SUCCESS;
}
