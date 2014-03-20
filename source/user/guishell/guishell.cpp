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
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/driver.h>
#include <esc/dir.h>
#include <esc/thread.h>
#include <gui/application.h>
#include <gui/window.h>
#include <gui/scrollpane.h>
#include <ipc/proto/vterm.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <shell/shell.h>
#include <shell/history.h>

#include "shellcontrol.h"
#include "guiterm.h"

#define DEF_COLS			80
#define DEF_ROWS			25

using namespace gui;
using namespace std;

static int guiProc(const char *devName);
static int shellMain(const char *devName);

static GUIVTermDevice *gt;

static void sigUsr2(A_UNUSED int sig) {
	Application::getInstance()->exit();
}

static void termSigUsr2(A_UNUSED int sig) {
	gt->stop();
}

static int termThread(A_UNUSED void *arg) {
	if(signal(SIG_USR2,termSigUsr2) == SIG_ERR)
		error("Unable to set signal-handler");
	gt->loop();
	return 0;
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

	// use a local path to have a unique name
	char devName[MAX_PATH_LEN];
	snprintf(devName,sizeof(devName),"/system/processes/%d/guiterm",getpid());

	// set term as env-variable
	setenv("TERM",devName);

	// start the shell in a separate process. this way, the forks the shell performs
	// are cheaper because its address-space is smaller.
	int childPid;
	if((childPid = fork()) == 0)
		return shellMain(devName);
	else if(childPid < 0)
		error("fork failed");

	guiProc(devName);
	return EXIT_SUCCESS;
}

static int guiProc(const char *devName) {
	if(signal(SIG_USR2,sigUsr2) == SIG_ERR)
		error("Unable to set signal-handler");

	// now start GUI
	Application *app = Application::create(getenv("WINMNG"));
	shared_ptr<Window> w = make_control<Window>("Shell",Pos(100,100));
	shared_ptr<Panel> root = w->getRootPanel();
	root->getTheme().setPadding(0);
	shared_ptr<ShellControl> sh = make_control<ShellControl>();
	gt = new GUIVTermDevice(devName,0777,sh,DEF_COLS,DEF_ROWS);
	int tid;
	if((tid = startthread(termThread,gt)) < 0)
		error("Unable to start term-thread");
	root->setLayout(make_layout<BorderLayout>());
	root->add(make_control<ScrollPane>(sh),BorderLayout::CENTER);
	w->show(true);
	w->requestFocus(sh.get());
	app->addWindow(w);
	int res = app->run();
	sh->sendEOF();
	// notify the other thread and wait for him
	if(kill(getpid(),SIG_USR2) < 0)
		printe("Unable to send SIG_USR2 to ourself");
	join(tid);
	delete gt;
	return res;
}

static int shellMain(const char *devName) {
	// wait until the device is announced
	int fin;
	do {
		fin = open(devName,IO_READ | IO_MSGS);
		if(fin < 0)
			yield();
	}
	while(fin < 0);

	// redirect fds so that stdin, stdout and stderr refer to our device
	if(redirect(STDIN_FILENO,fin) < 0)
		error("Unable to redirect STDIN to %d",fin);
	int fout = open(devName,IO_WRITE | IO_MSGS);
	if(fout < 0)
		error("Unable to open '%s' for writing",devName);
	if(redirect(STDOUT_FILENO,fout) < 0)
		error("Unable to redirect STDOUT to %d",fout);
	if(redirect(STDERR_FILENO,fout) < 0)
		error("Unable to redirect STDERR to %d",fout);

	// give vterm our pid
	{
		ipc::VTerm vterm(fin);
		vterm.setShellPid(getpid());
	}

	printf("\033[co;9]Welcome to Escape v%s!\033[co]\n",ESCAPE_VERSION);
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	int res = EXIT_SUCCESS;
	while(1) {
		// create buffer (history will free it)
		char *buffer = (char*)malloc((MAX_CMD_LEN + 1) * sizeof(char));
		if(buffer == nullptr)
			error("Not enough memory");

		if(!shell_prompt()) {
			res = EXIT_FAILURE;
			break;
		}

		/* read command */
		res = shell_readLine(buffer,MAX_CMD_LEN);
		if(res < 0)
			error("Unable to read from STDIN");
		if(res == 0)
			break;

		// execute it
		shell_executeCmd(buffer,false);
		shell_addToHistory(buffer);
	}

	// notify the parent that we're done
	if(kill(getppid(),SIG_USR2) < 0)
		printe("Unable to send SIG_USR2 to parent");
	return res;
}
