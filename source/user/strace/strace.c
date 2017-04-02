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
#include <sys/thread.h>
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
	fprintf(stderr,"Usage: %s [-o <file>] <program> [arguments...]\n",name);
	exit(EXIT_FAILURE);
}

static int waitingPid = 0;

int main(int argc,char **argv) {
	char path[MAX_PATH_LEN + 1] = "/bin/";
	if(argc < 2 || getopt_ishelp(argc,argv))
		usage(argv[0]);

	strcat(path,argv[1]);
	if(signal(SIGINT,sigHdlr) == SIG_ERR)
		error("Unable to set sig-handler for signal %d",SIGINT);

	/* create sync file and associate STRACE_FILENO with it */
	close(STRACE_FILENO);
	FILE *sync = tmpfile();
	if(!sync)
		error("Unable to create sync file");
	if(fileno(sync) != STRACE_FILENO)
		error("Got wrong fd for strace sync file");

	/* determine output file */
	int first = 1;
	int trc = STDERR_FILENO;
	if(strcmp(argv[1],"-o") == 0) {
		trc = creat(argv[2],0644);
		if(trc < 0)
			error("Unable to open %s for writing",argv[2]);
		first += 2;
	}

	if((waitingPid = fork()) == 0) {
		char tmp[12];

		/* set sync file */
		uint32_t cnt = 0;
		if(write(STRACE_FILENO,&cnt,sizeof(cnt)) != sizeof(cnt))
			error("Write to sync file failed");
		itoa(tmp,sizeof(tmp),STRACE_FILENO);
		setenv("SYSCTRC_SYNCFD",tmp);

		/* set output file */
		itoa(tmp,sizeof(tmp),trc);
		setenv("SYSCTRC_FD",tmp);

		size_t i;
		const char **args = (const char**)malloc(sizeof(char*) * (argc - first));
		if(!args)
			error("Not enough mem");
		for(i = first; i < (size_t)argc; i++)
			args[i - first] = argv[i];
		args[argc - first] = NULL;
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
		if(res < 0)
			error("Wait failed");
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
