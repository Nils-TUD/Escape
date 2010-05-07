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
#include <esc/io.h>
#include <stdio.h>
#include <signal.h>
#include <esc/cmdargs.h>
#include <string.h>

#define SIG_NAME_LEN		7

typedef struct {
	char name[SIG_NAME_LEN + 1];
	tSig signal;
} sSigName;

/* the signal the user can send */
static sSigName signals[] = {
	{"SIGKILL",SIG_KILL},
	{"SIGTERM",SIG_TERM},
	{"SIGINT",SIG_INTRPT}
};

int main(int argc,char **argv) {
	tPid pid;
	tSig signal = SIG_KILL;

	if(argc < 2 || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s [-L][-<signal>] <pid>\n",argv[0]);
		return EXIT_FAILURE;
	}

	/* print signals */
	if(strcmp(argv[1],"-L") == 0) {
		u32 i;
		for(i = 0; i < ARRAY_SIZE(signals); i++)
			printf("%10s - %d\n",signals[i].name,signals[i].signal);
		return EXIT_SUCCESS;
	}

	/* the user has specified the signal */
	if(argv[1][0] == '-') {
		u32 i;
		for(i = 0; i < ARRAY_SIZE(signals); i++) {
			if(strcmp(argv[1] + 1,signals[i].name) == 0 ||
				strcmp(argv[1] + 1,signals[i].name + 3) == 0) {
				signal = signals[i].signal;
				break;
			}
		}
		pid = atoi(argv[2]);
	}
	else
		pid = atoi(argv[1]);

	if(pid > 0) {
		if(sendSignalTo(pid,signal,0) < 0)
			error("Unable to send signal %d to %d",signal,pid);
	}
	else if(strcmp(argv[1],"0") != 0)
		fprintf(stderr,"Unable to kill process with pid '%s'\n",argv[1]);
	else
		fprintf(stderr,"You can't kill 'init'\n");

	return EXIT_SUCCESS;
}
