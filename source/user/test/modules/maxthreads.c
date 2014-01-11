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
#include <stdlib.h>
#include <stdio.h>

#include "../modules.h"

static int threadEntry(void *arg);

static int rdysm;
static int finsm;

int mod_maxthreads(A_UNUSED int argc,A_UNUSED char *argv[]) {
	rdysm = semcrt(0);
	finsm = semcrt(0);
	if(rdysm < 0 || finsm < 0)
		error("Unable to create semaphore(s)");

	int i = 0;
	while(true) {
		if(startthread(threadEntry,NULL) < 0) {
			printe("Unable to start thread");
			break;
		}
		i++;
	}

	printf("Started %zu threads; wait until all are running...\n",i);
	fflush(stdout);
	for(int j = 0; j < i; ++j)
		semdown(rdysm);

	printf("Signal them...\n");
	fflush(stdout);
	for(int j = 0; j < i; ++j)
		semup(finsm);

	printf("Waiting until all are done...\n");
	fflush(stdout);
	join(0);
	return EXIT_SUCCESS;
}

static int threadEntry(A_UNUSED void *arg) {
	semup(rdysm);
	semdown(finsm);
	return 0;
}
