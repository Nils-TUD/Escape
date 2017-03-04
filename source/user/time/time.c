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
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void sigHdlr(int sig);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <program> [arguments...]\n",name);
	exit(EXIT_FAILURE);
}

static int waitingPid = 0;

int main(int argc,char **argv) {
	uint64_t start,end;
	if(argc < 2 || getopt_ishelp(argc,argv))
		usage(argv[0]);

	if(signal(SIGINT,sigHdlr) == SIG_ERR)
		error("Unable to set sig-handler for signal %d",SIGINT);

	start = rdtsc();
	if((waitingPid = fork()) == 0) {
		size_t i;
		const char **args = (const char**)malloc(sizeof(char*) * argc);
		if(!args)
			error("Not enough mem");
		for(i = 1; i < (size_t)argc; i++)
			args[i - 1] = argv[i];
		args[argc - 1] = NULL;
		execvp(args[0],args);
		error("Exec failed");
	}
	else if(waitingPid < 0)
		error("Fork failed");
	else {
		sExitState state;
		int res;
		while(1) {
			res = waitchild(&state,-1,0);
			if(res != -EINTR)
				break;
		}
		end = rdtsc();
		if(res < 0)
			error("Wait failed");
		fprintf(stderr,"\n");
		fprintf(stderr,"Process %d ('",state.pid);
		for(int i = 1; i < argc; ++i) {
			fputs(argv[i],stderr);
			if(i + 1 < argc)
				fputc(' ',stderr);
		}
		fprintf(stderr,"') terminated with exit-code %d\n",state.exitCode);
		if(state.signal != SIG_COUNT)
			fprintf(stderr,"It was terminated by signal %d\n",state.signal);
		fprintf(stderr,"Runtime:		%Lu us\n",state.runtime);
		fprintf(stderr,"Realtime:		%Lu us\n",tsctotime(end - start));
		fprintf(stderr,"Scheduled:		%lu times\n",state.schedCount);
		fprintf(stderr,"Syscalls:		%lu\n",state.syscalls);
		fprintf(stderr,"Migrations:		%lu\n",state.migrations);
		fprintf(stderr,"Own mem:		%lu KiB\n",state.ownFrames * 4);
		fprintf(stderr,"Shared mem:		%lu KiB\n",state.sharedFrames * 4);
		fprintf(stderr,"Swapped:		%lu KiB\n",state.swapped * 4);
	}

	return EXIT_SUCCESS;
}

static void sigHdlr(A_UNUSED int sig) {
	if(waitingPid > 0) {
		/* send SIGINT to the child */
		if(kill(waitingPid,SIGINT) < 0)
			printe("Unable to send signal to %d",waitingPid);
	}
}
