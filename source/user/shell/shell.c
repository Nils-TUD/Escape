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
#include <esc/driver/vterm.h>
#include <esc/io.h>
#include <esc/cmdargs.h>
#include <esc/messages.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <shell/shell.h>
#include <shell/history.h>

static void usage(char *name) {
	fprintf(stderr,"Usage: %s [-e <command>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	long pid;
	char *buffer;

	/* we need either no args or "-e <cmd>" */
	if((argc != 1 && argc != 3) || isHelpCmd(argc,argv)) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	shell_init(argc,(const char**)argv);

	/* none-interactive-mode */
	if(argc == 3) {
		/* in this case we already have stdin, stdout and stderr */
		/* execute a script */
		if(strcmp(argv[1],"-s") == 0)
			return shell_executeCmd(argv[2],true);
		/* execute a command */
		if(strcmp(argv[1],"-e") == 0)
			return shell_executeCmd(argv[2],false);
		usage(argv[0]);
	}

	/* interactive mode */

	/* give vterm our pid */
	pid = getpid();
	if(vterm_setShellPid(STDOUT_FILENO,pid) < 0)
		error("Unable to send pid to vterm");

	while(1) {
		/* create buffer (history will free it) */
		buffer = (char*)malloc((MAX_CMD_LEN + 1) * sizeof(char));
		if(buffer == NULL)
			error("Not enough memory");

		if(!shell_prompt())
			return EXIT_FAILURE;

		/* read command */
		if(shell_readLine(buffer,MAX_CMD_LEN) < 0)
			error("Unable to read from STDIN");
		if(feof(stdin))
			break;

		/* execute it */
		shell_executeCmd(buffer,false);
		shell_addToHistory(buffer);
	}

	return EXIT_SUCCESS;
}
