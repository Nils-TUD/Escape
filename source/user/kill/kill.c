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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define SIG_NAME_LEN		7

typedef struct {
	char name[SIG_NAME_LEN + 1];
	int signal;
} sSigName;

/* the signal the user can send */
static sSigName signals[] = {
	{"SIGKILL",SIG_KILL},
	{"SIGTERM",SIG_TERM},
	{"SIGINT",SIG_INTRPT}
};

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-L] [-s <signal>] <pid>...\n",name);
	fprintf(stderr,"	<signal> may be: SIGKILL, SIGTERM, SIGINT, KILL, TERM, INT\n");
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	int sig = SIG_KILL;
	char *ssig = NULL;
	bool list = false;
	size_t i;

	int res = ca_parse(argc,argv,0,"L s=s",&list,&ssig);
	if(res < 0) {
		fprintf(stderr,"Invalid arguments: %s\n",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

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
			pid_t pid = atoi(*args);
			if(pid > 0) {
				if(kill(pid,sig) < 0)
					fprintf(stderr,"Unable to send signal %d to %d\n",sig,pid);
			}
			else if(strcmp(*args,"0") != 0)
				fprintf(stderr,"Unable to kill process with pid '%s'\n",*args);
			else
				fprintf(stderr,"You can't kill 'init'\n");
			args++;
		}
	}
	return EXIT_SUCCESS;
}
