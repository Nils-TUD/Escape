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
#include <sys/thread.h>
#include <sys/sync.h>
#include <stdio.h>
#include <stdlib.h>

#include "../modules.h"

#define RETRIES			1000
#define ITERATIONS		5000000
#define READERS			8
#define WRITERS			2

static tRWLock lck;
static volatile int curval = 0;
static int startsem;

static int thread_read(A_UNUSED void *arg) {
	semdown(startsem);
	for(int i = 0; i < RETRIES; ++i) {
		rwreq(&lck,RW_READ);
		int sum = 0;
		for(int j = 0; j < ITERATIONS; ++j)
			sum += curval;
		if(sum != ITERATIONS * curval)
			printe("Something went wrong. Sum should be %d, but is %d\n",ITERATIONS * curval,sum);
		rwrel(&lck,RW_READ);
	}
	return 0;
}

static int thread_write(A_UNUSED void *arg) {
	semdown(startsem);
	for(int i = 0; i < RETRIES; ++i) {
		rwreq(&lck,RW_WRITE);
		curval++;
		rwrel(&lck,RW_WRITE);
	}
	return 0;
}

int mod_rwlock(A_UNUSED int argc,A_UNUSED char *argv[]) {
	if((startsem = semcrt(READERS + WRITERS)) < 0)
		error("Unable to create start-sem");
	if(rwcrt(&lck) < 0)
		error("Unable to create rw-lock");

	for(int i = 0; i < WRITERS; ++i) {
		if(startthread(thread_write,NULL) < 0)
			error("Unable to start reader-thread");
	}
	for(int i = 0; i < READERS; ++i) {
		if(startthread(thread_read,NULL) < 0)
			error("Unable to start reader-thread");
	}
	/* go! */
	for(int i = 0; i < READERS + WRITERS; ++i)
		semup(startsem);
	join(0);

	if(curval != WRITERS * RETRIES)
		printe("Something went wrong. curval should be %d, but is %d\n",WRITERS * RETRIES,curval);

	rwdestr(&lck);
	return 0;
}
