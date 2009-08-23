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
#include <stdlib.h>

#include <shell/shell.h>
#include <shell/history.h>

#define MAX_VTERM_NAME_LEN	10

/**
 * Handles SIG_INTRPT
 */
static void shell_sigIntrpt(tSig sig,u32 data);

static u32 vterm;

int main(int argc,char **argv) {
	tFD fd;
	char *buffer;
	char servPath[9 + MAX_VTERM_NAME_LEN + 1] = "drivers:/";

	/* we need either the vterm as argument or "-e <cmd>" */
	if(argc < 2) {
		fprintf(stderr,
			"Usage: \n\tInteractive:\t\t%s <vterm>\n\tNon-Interactive:\t%s -e <yourCmd>\n",
			argv[0],argv[0]);
		return EXIT_FAILURE;
	}

	/* none-interactive-mode */
	if(argc == 3) {
		/* in this case we already have stdin, stdout and stderr */
		if(strcmp(argv[1],"-e") != 0) {
			fprintf(stderr,"Invalid shell-usage; Please use %s -e <cmd>\n",argv[1]);
			return EXIT_FAILURE;
		}

		return shell_executeCmd(argv[2]);
	}

	/* interactive mode */

	/* open stdin */
	strcat(servPath,argv[1]);
	/* parse vterm-number from "vtermX" */
	vterm = atoi(argv[1] + 5);
	if(open(servPath,IO_READ) < 0) {
		printe("Unable to open '%s' for STDIN",servPath);
		return EXIT_FAILURE;
	}

	/* open stdout */
	if((fd = open(servPath,IO_WRITE)) < 0) {
		printe("Unable to open '%s' for STDOUT",servPath);
		return EXIT_FAILURE;
	}

	/* dup stdout to stderr */
	if(dupFd(fd) < 0) {
		printe("Unable to duplicate STDOUT to STDERR");
		return EXIT_FAILURE;
	}

	if(setSigHandler(SIG_INTRPT,shell_sigIntrpt) < 0) {
		printe("Unable to announce sig-handler for %d",SIG_INTRPT);
		return EXIT_FAILURE;
	}

	/* set vterm as env-variable */
	setEnv("TERM",argv[1]);

	printf("\033f\011Welcome to Escape v0.1!\033r\011\n");
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	while(1) {
		/* create buffer (history will free it) */
		buffer = (char*)malloc((MAX_CMD_LEN + 1) * sizeof(char));
		if(buffer == NULL) {
			printf("Not enough memory\n");
			return EXIT_FAILURE;
		}

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
	/* was this interrupt intended for our vterm? */
	if(vterm == data) {
		printf("\n");
		tPid pid = shell_getWaitingPid();
		if(pid != INVALID_PID)
			sendSignalTo(pid,SIG_KILL,0);
		else
			shell_prompt();
	}
}
