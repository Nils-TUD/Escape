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
#include <esc/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../modules.h"

#define THREAD_COUNT	10

static tUserSem usem;

static int myThread(A_UNUSED void *arg) {
	size_t i;
	usemdown(&usem);
	printf("Thread %d starts\n",gettid());
	fflush(stdout);
	usemup(&usem);
	for(i = 0; i < 10; i++)
		sleep(100);
	usemdown(&usem);
	printf("Thread %d is done\n",gettid());
	fflush(stdout);
	usemup(&usem);
	return 0;
}

int mod_thread(A_UNUSED int argc,A_UNUSED char *argv[]) {
	int threads[THREAD_COUNT];
	if(usemcrt(&usem,1) < 0)
		error("Unable to create lock");

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
