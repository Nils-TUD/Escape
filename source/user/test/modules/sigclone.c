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
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../modules.h"

static pid_t parent = -1;
static pid_t child = -1;
static size_t parentCount = 0;
static size_t childCount = 0;

static void sigint(A_UNUSED int sig) {
	if(getpid() == parent) {
		parentCount++;
		kill(child,SIGINT);
	}
	else
		childCount++;
}

int mod_sigclone(A_UNUSED int argc,A_UNUSED char *argv[]) {
	int res;
	parent = getpid();
	if(signal(SIGINT,sigint) == SIG_ERR)
		error("Unable to set sighandler");

	if((child = fork()) == 0) {
		printf("Please give me a SIGINT!\n");
		fflush(stdout);
		sleep(1000);
		printf("Child got %d signals\n",childCount);
		exit(EXIT_SUCCESS);
	}

	/* parent waits */
	do {
		res = waitchild(NULL,-1,0);
	}
	while(res == -EINTR);
	printf("Parent got %d signals\n",parentCount);
	return 0;
}
