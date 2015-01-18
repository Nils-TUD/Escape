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

#include <esc/proto/vterm.h>
#include <shell/history.h>
#include <shell/shell.h>
#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(char *name) {
	fprintf(stderr,"Usage: %s [-e <command>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
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
	esc::VTerm vterm(STDOUT_FILENO);
	vterm.setShellPid(getpid());

	while(1) {
		size_t width = vterm.getMode().cols;
		/* create buffer (history will free it) */
		buffer = (char*)malloc((width + 1) * sizeof(char));
		if(buffer == NULL)
			error("Not enough memory");

		ssize_t count = shell_prompt();
		if(count < 0)
			return EXIT_FAILURE;

		/* read command */
		int res = shell_readLine(buffer,width - count);
		if(res < 0)
			error("Unable to read from STDIN");
		if(res == 0)
			break;

		/* execute it */
		shell_executeCmd(buffer,false);
		shell_addToHistory(buffer);
	}

	return EXIT_SUCCESS;
}
