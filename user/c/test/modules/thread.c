/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/lock.h>
#include <esc/thread.h>
#include <esc/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"

#define THREAD_COUNT	10

static tULock lck;

static int myThread(void *arg) {
	u32 i;
	UNUSED(arg);
	locku(&lck);
	printf("Thread %d starts\n",gettid());
	fflush(stdout);
	unlocku(&lck);
	for(i = 0; i < 10; i++)
		sleep(100);
	locku(&lck);
	printf("Thread %d is done\n",gettid());
	fflush(stdout);
	unlocku(&lck);
	return 0;
}

int mod_thread(int argc,char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	s32 threads[THREAD_COUNT];
	u32 i;
	for(i = 0; i < THREAD_COUNT; i++)
		assert((threads[i] = startThread(myThread,NULL)) >= 0);
	sleep(100);
	for(i = 0; i < THREAD_COUNT / 2; i++)
		assert(suspend(threads[i]) == 0);
	sleep(500);
	for(i = 0; i < THREAD_COUNT / 2; i++)
		assert(resume(threads[i]) == 0);
	for(i = 0; i < THREAD_COUNT; i++)
		join(threads[i]);
	assert(getThreadCount() == 1);
	return EXIT_SUCCESS;
}
