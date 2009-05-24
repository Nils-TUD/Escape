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
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/env.h>
#include <esc/signals.h>
#include <esc/service.h>
#include <esc/dir.h>
#include <esc/thread.h>
#include <stdlib.h>

#include <esc/gui/application.h>
#include <esc/gui/window.h>

#include <shell/shell.h>
#include <shell/history.h>

#include "shellcontrol.h"
#include "shellapp.h"

#define MAX_VTERM_NAME_LEN	10

using namespace esc::gui;

static char *servName;

/**
 * The shell-Thread
 */
static int shell_main(void);

/**
 * Handles SIG_INTRPT
 */
static void shell_sigIntrpt(tSig sig,u32 data);

int main(int argc,char **argv) {
	// none-interactive-mode
	if(argc == 3) {
		// in this case we already have stdin, stdout and stderr
		if(strcmp(argv[1],"-e") != 0) {
			fprintf(stderr,"Invalid shell-usage; Please use %s -e <cmd>\n",argv[1]);
			return EXIT_FAILURE;
		}

		return shell_executeCmd(argv[2]);
	}

	// announce service; try to find an unused service-name because maybe a user wants
	// to start us multiple times
	tServ sid;
	u32 no = 0;
	servName = new char[MAX_PATH_LEN + 1];
	do {
		sprintf(servName,"guiterm%d",no);
		sid = regService(servName,SERVICE_TYPE_SINGLEPIPE);
		if(sid >= 0)
			break;
		no++;
	}
	while(sid < 0);

	// redirect fds so that stdin, stdout and stderr refer to our service
	char *servPath = new char[MAX_PATH_LEN + 1];
	sprintf(servPath,"services:/guiterm%d",no);
	tFD fin = open(servPath,IO_READ);
	redirFd(STDIN_FILENO,fin);
	tFD fout = open(servPath,IO_WRITE);
	redirFd(STDOUT_FILENO,fout);
	redirFd(STDERR_FILENO,fout);
	delete servPath;

	// lets handle the shell-stuff in a separate thread
	/*if(fork() == 0)
		return shell_main();*/
	startThread(shell_main);

	// the parent handles the GUI
	ShellControl sh(0,0,500,280);
	ShellApplication *app = new ShellApplication(sid,no,&sh);
	Window w("Shell",100,100,500,300);
	w.add(sh);
	return app->run();
}

static int shell_main(void) {
	if(setSigHandler(SIG_INTRPT,shell_sigIntrpt) < 0) {
		printe("Unable to announce sig-handler for %d",SIG_INTRPT);
		return EXIT_FAILURE;
	}

	// set term as env-variable
	setEnv("TERM",servName);
	delete servName;

	printf("\033f\011Welcome to Escape v0.1!\033r\011\n");
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	char *buffer;
	while(1) {
		// create buffer (history will free it)
		buffer = (char*)malloc((MAX_CMD_LEN + 1) * sizeof(char));
		if(buffer == NULL) {
			printf("Not enough memory\n");
			return EXIT_FAILURE;
		}

		if(!shell_prompt())
			return EXIT_FAILURE;

		// read command
		shell_readLine(buffer,MAX_CMD_LEN);

		// execute it
		shell_executeCmd(buffer);
		shell_addToHistory(buffer);
	}
	return EXIT_SUCCESS;
}

static void shell_sigIntrpt(tSig sig,u32 data) {
	UNUSED(sig);
	printf("\n");
	tPid pid = shell_getWaitingPid();
	if(pid != INVALID_PID)
		sendSignalTo(pid,SIG_KILL,0);
	else
		shell_prompt();
}
