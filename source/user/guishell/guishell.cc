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
#include <gui/application.h>
#include <gui/scrollpane.h>
#include <gui/window.h>
#include <shell/history.h>
#include <shell/shell.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/version.h>
#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "guiterm.h"
#include "shellcontrol.h"

using namespace gui;
using namespace std;

static int guiProc(const char *devName);
static int shellMain(const char *devName);

static const int DEF_COLS = 80;
static const int DEF_ROWS = 25;

static GUIVTermDevice *gt;

static void sigUsr2(A_UNUSED int sig) {
	Application::getInstance()->exit();
}

static void termSigUsr2(A_UNUSED int sig) {
	gt->stop();
}

static int termThread(A_UNUSED void *arg) {
	if(signal(SIGUSR2,termSigUsr2) == SIG_ERR)
		error("Unable to set signal-handler");

	/* we haven't created it, so rebind it to ourself */
	gt->bindto(gettid());
	gt->loop();
	return 0;
}

int main(int argc,char **argv) {
	shell_init(argc,(const char**)argv);

	// use a local path to have a unique name
	// TODO that's not a long term solution
	char devName[MAX_PATH_LEN];
	snprintf(devName,sizeof(devName),"/sys/pid/%d/shm/guiterm",getpid());

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
	if(signal(SIGUSR2,sigUsr2) == SIG_ERR)
		error("Unable to set signal-handler");

	// now start GUI
	Application *app = Application::create(getenv("WINMNG"));
	auto w = make_control<Window>("Shell",Pos(100,100));
	auto root = w->getRootPanel();
	root->getTheme().setPadding(0);
	auto sh = make_control<ShellControl>();
	gt = new GUIVTermDevice(devName,0700,sh,DEF_COLS,DEF_ROWS);
	int tid;
	if((tid = startthread(termThread,gt)) < 0)
		error("Unable to start term-thread");
	root->setLayout(make_layout<BorderLayout>());
	root->add(make_control<ScrollPane>(sh),BorderLayout::CENTER);
	w->show(true);
	w->requestFocus(sh.get());
	app->addWindow(w);

	int res = 1;
	try {
		res = app->run();
	}
	catch(...) {
	}

	sh->sendEOF();
	// notify the other thread and wait for him
	if(kill(getpid(),SIGUSR2) < 0)
		printe("Unable to send SIGUSR2 to ourself");
	IGNSIGS(join(tid));
	delete gt;
	return res;
}

static int shellMain(const char *devName) {
	// wait until the device is announced
	int fin;
	do {
		fin = open(devName,O_RDONLY | O_MSGS);
		if(fin < 0)
			yield();
	}
	while(fin < 0);

	// redirect fds so that stdin, stdout and stderr refer to our device
	if(redirect(STDIN_FILENO,fin) < 0)
		error("Unable to redirect STDIN to %d",fin);
	int fout = open(devName,O_WRONLY | O_MSGS);
	if(fout < 0)
		error("Unable to open '%s' for writing",devName);
	if(redirect(STDOUT_FILENO,fout) < 0)
		error("Unable to redirect STDOUT to %d",fout);
	if(redirect(STDERR_FILENO,fout) < 0)
		error("Unable to redirect STDERR to %d",fout);

	// give vterm our pid
	esc::VTerm vterm(fin);

	printf("\033[co;9]Welcome to Escape v%s!\033[co]\n",ESCAPE_VERSION);
	printf("\n");
	printf("Try 'help' to see the current features :)\n");
	printf("\n");

	int res = EXIT_SUCCESS;
	while(1) {
		vterm.enableSignals();

		size_t width = vterm.getMode().cols;
		// create buffer (history will free it)
		char *buffer = (char*)malloc((width + 1) * sizeof(char));
		if(buffer == nullptr)
			error("Not enough memory");

		ssize_t count = shell_prompt();
		if(count < 0) {
			res = EXIT_FAILURE;
			break;
		}

		/* read command */
		res = shell_readLine(buffer,width - count);
		if(res < 0)
			error("Unable to read from STDIN");
		if(res == 0)
			break;

		// execute it
		shell_executeCmd(buffer,false);
		shell_addToHistory(buffer);
	}

	// notify the parent that we're done
	if(kill(getppid(),SIGUSR2) < 0)
		printe("Unable to send SIGUSR2 to parent");
	return res;
}
