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
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>

#include "../modules.h"

#define TEST_COUNT		1000

static void firenforget(void) {
	size_t i;
	uint64_t total = 0;
	for(i = 0; i < TEST_COUNT; ++i) {
		uint64_t start = rdtsc();
		int pid = fork();
		total += rdtsc() - start;
		if(pid == 0)
			exit(0);
		else if(pid < 0) {
			printe("fork failed");
			return;
		}
	}
	printf("fork      : %Lu cycles/call\n",total / TEST_COUNT);

	/* better catch the zombies now */
	for(i = 0; i < TEST_COUNT; ++i)
		waitchild(NULL,-1);
}

static void waitdead(void) {
	size_t i;
	uint64_t total = 0;
	for(i = 0; i < TEST_COUNT; ++i) {
		uint64_t start = rdtsc();
		int pid = fork();
		if(pid == 0)
			exit(0);
		else if(pid < 0) {
			printe("fork failed");
			return;
		}
		waitchild(NULL,-1);
		total += rdtsc() - start;
	}
	printf("fork      : %Lu cycles/call\n",total / TEST_COUNT);
}

int mod_fork(A_UNUSED int argc,A_UNUSED char *argv[]) {
	printf("Fire and forget...\n");
	fflush(stdout);
	firenforget();
	printf("Wait until they're dead...\n");
	fflush(stdout);
	waitdead();
	return EXIT_SUCCESS;
}
