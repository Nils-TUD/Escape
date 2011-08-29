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
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errors.h>

static void sigTimer(int sig);
static void sigHdlr(int sig);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <program> [arguments...]\n",name);
	exit(EXIT_FAILURE);
}

static int timerFreq;
static time_t ms = 0;
static int waitingPid = 0;

int main(int argc,char **argv) {
	char path[MAX_PATH_LEN + 1] = "/bin/";
	if(argc < 2 || isHelpCmd(argc,argv))
		usage(argv[0]);

	timerFreq = getConf(CONF_TIMER_FREQ);
	if(timerFreq < 0)
		error("Unable to get timer-frequency");

	strcat(path,argv[1]);
	if(setSigHandler(SIG_INTRPT,sigHdlr) < 0)
		error("Unable to set sig-handler for signal %d",SIG_INTRPT);
	if(setSigHandler(SIG_INTRPT_TIMER,sigTimer) < 0)
		error("Unable to set sig-handler for signal %d",SIG_INTRPT_TIMER);

	if((waitingPid = fork()) == 0) {
		int i;
		size_t size = 1,pos = 0;
		const char *args[] = {"/bin/shell","-e",NULL,NULL};
		char *arg = NULL;
		for(i = 1; i < argc; i++) {
			size_t len = strlen(argv[i]);
			size += len + 1;
			arg = (char*)realloc(arg,size);
			if(!arg)
				error("Not enough memory for arguments");
			strcpy(arg + pos,argv[i]);
			pos += len;
			arg[pos++] = ' ';
			arg[pos] = '\0';
		}
		args[2] = arg;
		exec(args[0],args);
		error("Exec failed");
	}
	else if(waitingPid < 0)
		error("Fork failed");
	else {
		sExitState state;
		int res;
		while(1) {
			res = waitChild(&state);
			if(res != ERR_INTERRUPTED)
				break;
		}
		if(res < 0)
			error("Wait failed");
		if(setSigHandler(SIG_INTRPT_TIMER,SIG_DFL) < 0)
			error("Unable to unset signal-handler");
		printf("\n");
		printf("Process %d (%s) terminated with exit-code %d\n",state.pid,path,state.exitCode);
		if(state.signal != SIG_COUNT)
			printf("It was terminated by signal %d\n",state.signal);
		printf("Total time:		%u ms\n",ms);
		printf("Runtime:		%u ms\n",state.runtime);
		printf("Own mem:		%lu KiB\n",state.ownFrames * 4);
		printf("Shared mem:		%lu KiB\n",state.sharedFrames * 4);
		printf("Swap mem:		%lu KiB\n",state.swapped * 4);
		printf("Scheduled:		%lu times\n",state.schedCount);
		printf("Syscalls:		%lu\n",state.syscalls);
	}

	return EXIT_SUCCESS;
}

static void sigTimer(int sig) {
	UNUSED(sig);
	ms += 1000 / timerFreq;
}

static void sigHdlr(int sig) {
	UNUSED(sig);
	if(waitingPid > 0) {
		/* send SIG_INTRPT to the child */
		if(sendSignalTo(waitingPid,SIG_INTRPT) < 0)
			printe("Unable to send signal to %d",waitingPid);
	}
}
