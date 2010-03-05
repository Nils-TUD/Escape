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
#include <esc/driver.h>
#include <esc/dir.h>
#include <esc/thread.h>
#include <esc/lock.h>
#include <stdlib.h>

#include <esc/gui/application.h>
#include <esc/gui/window.h>

#include <shell/shell.h>
#include <shell/history.h>

#include "shellcontrol.h"
#include "shellapp.h"

#define GUI_SHELL_LOCK		0x4129927
#define MAX_VTERM_NAME_LEN	10

using namespace esc::gui;

static char *drvName;

/**
 * The shell-Thread
 */
static int shell_main(void);

int main(int argc,char **argv) {
	shell_init();

	// none-interactive-mode
	if(argc == 3) {
		// in this case we already have stdin, stdout and stderr
		// execute a script
		if(strcmp(argv[1],"-s") == 0)
			return shell_executeCmd(argv[2],true);
		// execute a command
		if(strcmp(argv[1],"-e") == 0)
			return shell_executeCmd(argv[2],false);
		fprintf(stderr,"Invalid shell-usage; Please use %s -e <cmd>\n",argv[1]);
		return EXIT_FAILURE;
	}

	// use a lock here to ensure that no one uses our guiterm-number
	lockg(GUI_SHELL_LOCK);

	// announce driver; try to find an unused driver-name because maybe a user wants
	// to start us multiple times
	tDrvId sid;
	u32 no = 0;
	drvName = new char[MAX_PATH_LEN + 1];
	do {
		snprintf(drvName,MAX_PATH_LEN + 1,"guiterm%d",no);
		sid = regDriver(drvName,DRV_READ | DRV_WRITE | DRV_TERM);
		if(sid >= 0)
			break;
		no++;
	}
	while(sid < 0);
	unregDriver(sid);

	// set term as env-variable
	setEnv("TERM",drvName);

	// the child handles the GUI
	if(fork() == 0) {
		// re-register driver
		sid = regDriver(drvName,DRV_READ | DRV_WRITE | DRV_TERM);
		unlockg(GUI_SHELL_LOCK);
		if(sid < 0)
			error("Unable to re-register driver %s",drvName);
		delete drvName;

		// now start GUI
		ShellControl *sh = new ShellControl(sid,0,0,700,480);
		ShellApplication *app = new ShellApplication(sid,sh);
		Window w("Shell",100,100,700,500);
		w.add(*sh);
		return app->run();
	}

	// wait until the driver is announced
	delete drvName;
	char *drvPath = new char[MAX_PATH_LEN + 1];
	snprintf(drvPath,MAX_PATH_LEN + 1,"/dev/guiterm%d",no);
	tFD fin;
	do {
		fin = open(drvPath,IO_READ);
		if(fin < 0)
			yield();
	}
	while(fin < 0);

	// redirect fds so that stdin, stdout and stderr refer to our driver
	if(redirFd(STDIN_FILENO,fin) < 0)
		error("Unable to redirect STDIN to %d",fin);
	tFD fout = open(drvPath,IO_WRITE);
	if(fout < 0)
		error("Unable to open '%s' for writing",drvPath);
	if(redirFd(STDOUT_FILENO,fout) < 0)
		error("Unable to redirect STDOUT to %d",fout);
	if(redirFd(STDERR_FILENO,fout) < 0)
		error("Unable to redirect STDERR to %d",fout);
	delete drvPath;

	/* give vterm our pid */
	sendMsgData(fin,MSG_VT_SHELLPID,(u8*)getpid(),sizeof(tPid));

	return shell_main();
}

static int shell_main(void) {
	printf("\033[co;9]Welcome to Escape v0.3!\033[co]\n");
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	char *buffer;
	while(1) {
		// create buffer (history will free it)
		buffer = (char*)malloc((MAX_CMD_LEN + 1) * sizeof(char));
		if(buffer == NULL)
			error("Not enough memory");

		if(!shell_prompt())
			return EXIT_FAILURE;

		// read command
		shell_readLine(buffer,MAX_CMD_LEN);

		// execute it
		shell_executeCmd(buffer,false);
		shell_addToHistory(buffer);
	}
	return EXIT_SUCCESS;
}
