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
#include <esc/proc.h>
#include <esc/thread.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "forkbomb.h"

#define MAX_PIDS	8192

static int pids[MAX_PIDS];

int mod_forkbomb(int argc,char *argv[]) {
	ssize_t n = argc > 2 ? atoi(argv[2]) : 100;
	ssize_t i = 0;
	while(1) {
		pids[i] = fork();
		/* failed? so send all created child-procs the kill-signal */
		if(i >= n || pids[i] < 0) {
			if(pids[i] < 0)
				printe("fork() failed");
			printf("Kill all childs\n");
			fflush(stdout);
			while(i >= 0) {
				if(pids[i] > 0) {
					sendSignalTo(pids[i],SIG_KILL);
					waitChild(NULL);
				}
				i--;
			}
			return 0;
		}
		/* the childs break here */
		if(pids[i] == 0)
			break;
		i++;
	}

	wait(EV_NOEVENT,0);
	return 0;
}
