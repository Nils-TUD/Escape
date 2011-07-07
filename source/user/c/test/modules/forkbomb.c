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

#define MAX_PIDS	2048

int pids[MAX_PIDS];

int mod_forkbomb(int argc,char *argv[]) {
	size_t n = argc > 2 ? atoi(argv[2]) : 100;
	size_t i = 0;
	while(1) {
		pids[i] = fork();
		/* failed? so send all created child-procs the kill-signal */
		if(i >= n || pids[i] < 0) {
			printf("Fork() failed, so kill all childs...\n");
			fflush(stdout);
			while(i-- > 0) {
				int res = sendSignalTo(pids[i],SIG_KILL);
				res = 1;
				waitChild(NULL);
			}
			printf("Done :)\n");
			return 0;
		}
		/* the childs break here */
		if(pids[i] == 0)
			break;
		i++;
	}

	/* now stay here until we get killed ^^ */
	printf("Child %d running...\n",getpid());
	fflush(stdout);
	while(1)
		sleep(1000);
	return 0;
}
