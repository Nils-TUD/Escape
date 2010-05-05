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
#include <esc/proc.h>
#include <stdio.h>
#include <esc/signals.h>
#include <string.h>
#include "forkbomb.h"

#define MAX_PIDS	2048

s32 pids[MAX_PIDS];

int mod_forkbomb(int argc,char *argv[]) {
	u32 n = argc > 2 ? atoi(argv[2]) : 100;
	u32 i = 0;
	while(1) {
		pids[i] = fork();
		/* failed? so send all created child-procs the kill-signal */
		if(i >= n || pids[i] < 0) {
			printf("Fork() failed, so kill all childs...\n");
			fflush(stdout);
			while(i-- > 0) {
				s32 res = sendSignalTo(pids[i],SIG_KILL,0);
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
