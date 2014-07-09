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
#include <esc/esccodes.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/sync.h>
#include <esc/conf.h>
#include <stdio.h>
#include <stdlib.h>

#include "keystrokes.h"
#include "jobmng.h"
#include "screens.h"

std::mutex Keystrokes::mutex;

const int Keystrokes::VGA_MODE			= 3;

const char *Keystrokes::VTERM_PROG		= "/sbin/vterm";
const char *Keystrokes::LOGIN_PROG		= "/bin/login";

const char *Keystrokes::WINMNG_PROG		= "/sbin/winmng";
const char *Keystrokes::GLOGIN_PROG		= "/bin/glogin";

const char *Keystrokes::TUI_DEF_COLS	= "100";
const char *Keystrokes::TUI_DEF_ROWS	= "37";

const char *Keystrokes::GUI_DEF_RES_X	= "800";
const char *Keystrokes::GUI_DEF_RES_Y	= "600";

void Keystrokes::createConsole(const char *mng,const char *cols,const char *rows,const char *login,
							   const char *termVar) {
	char name[SSTRLEN("ui") + 11];
	char path[SSTRLEN("/dev/ui") + 11];

	std::lock_guard<std::mutex> guard(mutex);
	int id = JobMng::getId();
	if(id < 0) {
		printe("Maximum number of clients reached");
		return;
	}

	snprintf(name,sizeof(name),"ui%d",id);
	snprintf(path,sizeof(path),"/dev/%s",name);

	print("Starting '%s'",name);

	int mngPid = fork();
	if(mngPid < 0) {
		printe("fork failed");
		return;
	}
	if(mngPid == 0) {
		/* close all but stdin, stdout, stderr */
		int max = sysconf(CONF_MAX_FDS);
		for(int i = 3; i < max; ++i)
			close(i);

		print("Executing %s %s %s %s",mng,cols,rows,name);
		const char *args[] = {mng,cols,rows,name,NULL};
		execv(mng,args);
		error("exec with %s failed",mng);
	}

	JobMng::add(id,mngPid);

	print("Waiting for %s",path);
	/* TODO not good */
	int fd;
	while((fd = open(path,O_MSGS)) < 0) {
		if(fd != -ENOENT)
			printe("Unable to open '%s'",path);
		if(!JobMng::exists(id))
			return;
		sleep(20);
	}
	close(fd);

	int loginPid = fork();
	if(loginPid < 0) {
		printe("fork failed");
		return;
	}
	if(loginPid == 0) {
		/* close all; login will open different streams */
		int max = sysconf(CONF_MAX_FDS);
		for(int i = 0; i < max; ++i)
			close(i);

		/* set env-var for childs */
		setenv(termVar,path);

		print("Executing %s",login);
		const char *args[] = {login,NULL};
		execv(login,args);
		error("exec with %s failed",login);
	}

	JobMng::setLoginPid(id,loginPid);
}

void Keystrokes::switchToVGA() {
	ipc::Screen *scr;
	ipc::Screen::Mode mode;
	if(ScreenMng::find(VGA_MODE,&mode,&scr)) {
		try {
			scr->setMode(ipc::Screen::MODE_TYPE_TUI,VGA_MODE,"",true);
		}
		catch(const std::exception &e) {
			printe("Unable to switch to VGA: %s",e.what());
		}
	}
	else
		printe("Unable to find screen for mode %d",VGA_MODE);
}
