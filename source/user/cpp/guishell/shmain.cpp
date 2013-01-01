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
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/driver.h>
#include <esc/dir.h>
#include <esc/thread.h>
#include <gui/application.h>
#include <gui/window.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <shell/shell.h>
#include <shell/history.h>

#include "shellcontrol.h"
#include "guiterm.h"

#define GUI_SHELL_LOCK		0x4129927
#define MAX_VTERM_NAME_LEN	10
#define DEF_COLS			80
#define DEF_ROWS			30

using namespace gui;

static int guiProc(void);
static int termThread(void *arg);
static int shellMain(void);

static char *drvName;
static GUITerm *gt;
static int childPid;

static void sigUsr1(A_UNUSED int sig) {
	Application::getInstance()->exit();
	gt->stop();
}

int main(int argc,char **argv) {
	shell_init(argc,(const char**)argv);

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
	lockg(GUI_SHELL_LOCK,LOCK_EXCLUSIVE);

	// announce device; try to find an unused device-name because maybe a user wants
	// to start us multiple times
	int sid;
	size_t no = 0;
	drvName = new char[MAX_PATH_LEN + 1];
	do {
		snprintf(drvName,MAX_PATH_LEN + 1,"/dev/guiterm%d",no);
		sid = createdev(drvName,DEV_TYPE_CHAR,DEV_READ | DEV_WRITE);
		if(sid >= 0)
			break;
		no++;
	}
	while(sid < 0);
	close(sid);

	// set term as env-variable
	setenv("TERM",drvName);

	// start the gui and the device in a separate process. this way, the forks the shell performs
	// are cheaper because its address-space is smaller.
	if((childPid = fork()) == 0)
		return guiProc();
	else if(childPid < 0)
		error("fork failed");

	// wait until the device is announced
	char *drvPath = new char[MAX_PATH_LEN + 1];
	snprintf(drvPath,MAX_PATH_LEN + 1,"/dev/guiterm%d",no);
	int fin;
	do {
		fin = open(drvPath,IO_READ | IO_MSGS);
		if(fin < 0)
			yield();
	}
	while(fin < 0);

	// redirect fds so that stdin, stdout and stderr refer to our device
	if(redirect(STDIN_FILENO,fin) < 0)
		error("Unable to redirect STDIN to %d",fin);
	int fout = open(drvPath,IO_WRITE | IO_MSGS);
	if(fout < 0)
		error("Unable to open '%s' for writing",drvPath);
	if(redirect(STDOUT_FILENO,fout) < 0)
		error("Unable to redirect STDOUT to %d",fout);
	if(redirect(STDERR_FILENO,fout) < 0)
		error("Unable to redirect STDERR to %d",fout);
	delete[] drvPath;

	// give vterm our pid
	long pid = getpid();
	if(vterm_setShellPid(fin,pid) < 0)
		error("Unable to set shell-pid");

	shellMain();

	// notify the child that we're done
	if(kill(childPid,SIG_USR1) < 0)
		printe("Unable to send SIG_USR1 to child");
	return EXIT_SUCCESS;
}

static int guiProc(void) {
	// re-register device
	int sid = createdev(drvName,DEV_TYPE_CHAR,DEV_READ | DEV_WRITE);
	unlockg(GUI_SHELL_LOCK);
	if(sid < 0)
		error("Unable to re-register device %s",drvName);
	delete[] drvName;

	if(signal(SIG_USR1,sigUsr1) == SIG_ERR)
		error("Unable to set signal-handler");

	// now start GUI
	Application *app = Application::getInstance();
	Font font;
	Window w("Shell",Pos(100,100),Size(font.getSize().width * DEF_COLS + 2,
								  	   font.getSize().height * DEF_ROWS + 4));
	Panel& root = w.getRootPanel();
	root.getTheme().setPadding(0);
	ShellControl *sh = new ShellControl(Pos(0,0),root.getSize());
	gt = new GUITerm(sid,sh);
	if(startthread(termThread,gt) < 0)
		error("Unable to start term-thread");
	root.setLayout(new BorderLayout());
	root.add(sh,BorderLayout::CENTER);
	w.show();
	w.setFocus(sh);
	int res = app->run();
	sh->sendEOF();
	return res;
}

static int termThread(A_UNUSED void *arg) {
	if(signal(SIG_USR1,sigUsr1) == SIG_ERR)
		error("Unable to set signal-handler");
	gt->run();
	return 0;
}

static int shellMain(void) {
	printf("\033[co;9]Welcome to Escape v%s!\033[co]\n",ESCAPE_VERSION);
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	/* TODO temporary */
	setenv("USER","hrniels");

	while(1) {
		// create buffer (history will free it)
		char *buffer = (char*)malloc((MAX_CMD_LEN + 1) * sizeof(char));
		if(buffer == NULL)
			error("Not enough memory");

		if(!shell_prompt())
			return EXIT_FAILURE;

		// read command
		if(shell_readLine(buffer,MAX_CMD_LEN) < 0)
			error("Unable to read from STDIN");
		if(feof(stdin))
			break;

		// execute it
		shell_executeCmd(buffer,false);
		shell_addToHistory(buffer);
	}
	return EXIT_SUCCESS;
}
