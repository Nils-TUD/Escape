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
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "../modules.h"

#define LOCK_IDENT	0x44112241

static void sigUsr1(A_UNUSED int sig);
static int threadEntry(A_UNUSED void *arg);

static size_t threadsRunning;

int mod_maxthreads(A_UNUSED int argc,A_UNUSED char *argv[]) {
	size_t i = 0;
	while(true) {
		if(startthread(threadEntry,NULL) < 0) {
			printe("Unable to start thread");
			break;
		}
		i++;
	}

	printf("Started %zu threads; wait until all are running...\n",i);
	fflush(stdout);
	while(true) {
		lock(LOCK_IDENT,LOCK_EXCLUSIVE);
		if(threadsRunning == i) {
			unlock(LOCK_IDENT);
			break;
		}
		unlock(LOCK_IDENT);
		sleep(20);
	}
	printf("Kill them...\n");
	fflush(stdout);
	if(kill(getpid(),SIG_USR1) < 0)
		perror("kill");
	return EXIT_SUCCESS;
}

static void sigUsr1(A_UNUSED int sig) {
	/* do nothing */
}

static int threadEntry(A_UNUSED void *arg) {
	lock(LOCK_IDENT,LOCK_EXCLUSIVE);
	threadsRunning++;
	if(signal(SIG_USR1,sigUsr1) == SIG_ERR) {
		unlock(LOCK_IDENT);
		error("Unable to announce signal-handler");
	}
	waitunlock(EV_NOEVENT,0,LOCK_IDENT);
	return 0;
}
