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
#include <esc/cmdargs.h>
#include <esc/proc.h>
#include <esc/dir.h>
#include <esc/conf.h>
#include <esc/time.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void sigHdlr(int sig);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <program> [arguments...]\n",name);
	exit(EXIT_FAILURE);
}

static int waitingPid = 0;

int main(int argc,char **argv) {
	uint64_t start,end;
	char path[MAX_PATH_LEN + 1] = "/bin/";
	if(argc < 2 || isHelpCmd(argc,argv))
		usage(argv[0]);

	strcat(path,argv[1]);
	if(signal(SIG_INTRPT,sigHdlr) == SIG_ERR)
		error("Unable to set sig-handler for signal %d",SIG_INTRPT);

	start = rdtsc();
	if((waitingPid = fork()) == 0) {
		size_t i;
		const char **args = (const char**)malloc(sizeof(char*) * (argc - 1));
		if(!args)
			error("Not enough mem");
		for(i = 1; i < (size_t)argc; i++)
			args[i - 1] = argv[i];
		args[argc - 1] = NULL;
		execp(args[0],args);
		error("Exec failed");
	}
	else if(waitingPid < 0)
		error("Fork failed");
	else {
		sExitState state;
		int res;
		while(1) {
			res = waitchild(&state);
			if(res != -EINTR)
				break;
		}
		end = rdtsc();
		if(res < 0)
			error("Wait failed");
		if(signal(SIG_INTRPT_TIMER,SIG_DFL) == SIG_ERR)
			error("Unable to unset signal-handler");
		printf("\n");
		printf("Process %d (%s) terminated with exit-code %d\n",state.pid,path,state.exitCode);
		if(state.signal != SIG_COUNT)
			printf("It was terminated by signal %d\n",state.signal);
		printf("Runtime:		%Lu us\n",state.runtime);
		printf("Realtime:		%Lu us\n",tsctotime(end - start));
		printf("Scheduled:		%lu times\n",state.schedCount);
		printf("Syscalls:		%lu\n",state.syscalls);
		printf("Migrations:		%lu\n",state.migrations);
		printf("Own mem:		%lu KiB\n",state.ownFrames * 4);
		printf("Shared mem:		%lu KiB\n",state.sharedFrames * 4);
		printf("Swapped:		%lu KiB\n",state.swapped * 4);
	}

	return EXIT_SUCCESS;
}

static void sigHdlr(A_UNUSED int sig) {
	if(waitingPid > 0) {
		/* send SIG_INTRPT to the child */
		if(kill(waitingPid,SIG_INTRPT) < 0)
			printe("Unable to send signal to %d",waitingPid);
	}
}
