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
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/env.h>
#include <esc/signals.h>
#include <esc/cmdargs.h>
#include <stdlib.h>

#include <shell/shell.h>
#include <shell/history.h>

#define MAX_VTERM_NAME_LEN	10

/**
 * Handles SIG_INTRPT
 */
static void shell_sigIntrpt(tSig sig,u32 data);

static u32 vterm;

static void usage(char *name) {
	fprintf(stderr,"Usage: \n");
	fprintf(stderr,"	Interactive:		%s <vterm>\n",name);
	fprintf(stderr,"	Non-Interactive:	%s -e <yourCmd>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	tFD fd;
	tPid pid;
	char *buffer;
	char servPath[SSTRLEN("/drivers/") + MAX_VTERM_NAME_LEN + 1] = "/drivers/";

	/* we need either the vterm as argument or "-e <cmd>" */
	if((argc != 2 && argc != 3) || isHelpCmd(argc,argv)) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if(setSigHandler(SIG_INTRPT,shell_sigIntrpt) < 0)
		error("Unable to announce sig-handler for %d",SIG_INTRPT);

	/* none-interactive-mode */
	if(argc == 3) {
		/* in this case we already have stdin, stdout and stderr */
		if(strcmp(argv[1],"-e") != 0)
			usage(argv[0]);

		return shell_executeCmd(argv[2]);
	}

	/* interactive mode */

	/* open stdin */
	strcat(servPath,argv[1]);
	/* parse vterm-number from "vtermX" */
	vterm = atoi(argv[1] + 5);
	if(open(servPath,IO_READ) < 0)
		error("Unable to open '%s' for STDIN",servPath);

	/* open stdout */
	if((fd = open(servPath,IO_WRITE)) < 0)
		error("Unable to open '%s' for STDOUT",servPath);

	/* dup stdout to stderr */
	if(dupFd(fd) < 0)
		error("Unable to duplicate STDOUT to STDERR");

	/* give vterm our pid */
	pid = getpid();
	ioctl(fd,IOCTL_VT_SHELLPID,(u8*)&pid,sizeof(tPid));

	/* set vterm as env-variable */
	setEnv("TERM",argv[1]);

	printf("\033[co;9]Welcome to Escape v0.2!\033[co]\n");
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	while(1) {
		/* create buffer (history will free it) */
		buffer = (char*)malloc((MAX_CMD_LEN + 1) * sizeof(char));
		if(buffer == NULL)
			error("Not enough memory");

		if(!shell_prompt())
			return EXIT_FAILURE;

		/* read command */
		shell_readLine(buffer,MAX_CMD_LEN);

		/* execute it */
		shell_executeCmd(buffer);
		shell_addToHistory(buffer);
	}

	return EXIT_SUCCESS;
}

static void shell_sigIntrpt(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	tPid pid = shell_getWaitingPid();
	printf("\n");
	if(pid != INVALID_PID)
		sendSignalTo(pid,SIG_INTRPT,0);
	else
		shell_prompt();
}
