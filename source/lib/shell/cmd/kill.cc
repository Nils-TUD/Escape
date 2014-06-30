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

#include <esc/common.h>
#include <esc/cmdargs.h>
#include <esc/dir.h>
#include <esc/io.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "../cmds.h"
#include "../ast/command.h"

#define SIG_NAME_LEN		7

typedef struct {
	char name[SIG_NAME_LEN + 1];
	int signal;
} sSigName;

/* the signal the user can send */
static sSigName signals[] = {
	{"SIGKILL",SIG_KILL},
	{"SIGTERM",SIG_TERM},
	{"SIGINT",SIG_INTRPT},
	{"KILL",SIG_KILL},
	{"TERM",SIG_TERM},
	{"INT",SIG_INTRPT},
};

static int usage(const char *name) {
	fprintf(stderr,"Usage: %s [-L] [-s <signal>] <pid>|<jobid>...\n",name);
	fprintf(stderr,"    -L: list available signals\n");
	return EXIT_FAILURE;
}

int shell_cmdKill(int argc,char **argv) {
	int sig = SIG_TERM;
	char *ssig = NULL;
	bool list = false;
	size_t i;

	int res = ca_parse(argc,(const char**)argv,0,"L s=s",&list,&ssig);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		return usage(argv[0]);
	}
	if((!list && ca_getFreeCount() == 0) || ca_hasHelp())
		return usage(argv[0]);

	/* translate signal-name to signal-number */
	if(ssig) {
		for(i = 0; i < ARRAY_SIZE(signals); i++) {
			if(strcmp(ssig,signals[i].name) == 0 ||
				strcmp(ssig,signals[i].name + 3) == 0) {
				sig = signals[i].signal;
				break;
			}
		}
	}

	/* print signals */
	if(list) {
		for(i = 0; i < ARRAY_SIZE(signals); i++)
			printf("%10s - %d\n",signals[i].name,signals[i].signal);
	}
	else {
		/* kill processes */
		const char **args = ca_getFree();
		while(*args) {
			if((*args)[0] == '%') {
				tJobId cmd = atoi(*args + 1);
				ast_termProcsOfJob(cmd);
			}
			else {
				pid_t pid = atoi(*args);
				if(pid > 0) {
					if(kill(pid,sig) < 0)
						printe("Unable to send signal %d to %d",sig,pid);
				}
				else if(strcmp(*args,"0") != 0)
					printe("Unable to kill process with pid '%s'",*args);
				else
					printe("You can't kill 'init'");
			}
			args++;
		}
	}
	return EXIT_SUCCESS;
}
