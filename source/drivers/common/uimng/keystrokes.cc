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
#include <esc/debug.h>
#include <esc/conf.h>
#include <stdio.h>
#include <stdlib.h>

#include "keystrokes.h"
#include "clients.h"
#include "jobs.h"
#include "screens.h"

#define VGA_MODE		3

#define VTERM_PROG		"/sbin/vterm"
#define LOGIN_PROG		"/bin/login"

#define WINMNG_PROG		"/sbin/winmng"
#define GLOGIN_PROG		"/bin/glogin"

#define TUI_DEF_COLS	"100"
#define TUI_DEF_ROWS	"37"

#define GUI_DEF_RES_X	"800"
#define GUI_DEF_RES_Y	"600"

static tUserSem usem;

void keys_init(void) {
	if(usemcrt(&usem,1) < 0)
		error("Unable to create keystrokes lock");
}

static void keys_createConsole(const char *mng,const char *cols,const char *rows,const char *login,
							   const char *termVar) {
	char name[SSTRLEN("ui") + 11];
	char path[SSTRLEN("/dev/ui") + 11];

	usemdown(&usem);
	int id = jobs_getId();
	if(id < 0) {
		printe("Maximum number of clients reached");
		usemup(&usem);
		return;
	}

	snprintf(name,sizeof(name),"ui%d",id);
	snprintf(path,sizeof(path),"/dev/%s",name);

	print("Starting '%s'",name);

	int mngPid = fork();
	if(mngPid < 0)
		goto errorFork;
	if(mngPid == 0) {
		/* close all but stdin, stdout, stderr */
		int max = sysconf(CONF_MAX_FDS);
		for(int i = 3; i < max; ++i)
			close(i);

		print("Executing %s %s %s %s",mng,cols,rows,name);
		const char *args[] = {mng,cols,rows,name,NULL};
		exec(mng,args);
		error("exec with %s failed",mng);
	}

	jobs_add(id,mngPid);

	print("Waiting for %s",path);
	/* TODO not good */
	int fd;
	while((fd = open(path,IO_MSGS)) < 0) {
		if(fd != -ENOENT)
			printe("Unable to open '%s'",path);
		if(!jobs_exists(id))
			goto errorOpen;
		sleep(20);
	}
	close(fd);

	{
		int loginPid = fork();
		if(loginPid < 0)
			goto errorFork;
		if(loginPid == 0) {
			/* close all; login will open different streams */
			int max = sysconf(CONF_MAX_FDS);
			for(int i = 0; i < max; ++i)
				close(i);

			/* set env-var for childs */
			setenv(termVar,path);

			print("Executing %s",login);
			const char *args[] = {login,NULL};
			exec(login,args);
			error("exec with %s failed",login);
		}

		jobs_setLoginPid(id,loginPid);
	}
	usemup(&usem);
	return;

errorFork:
	printe("fork failed");
errorOpen:
	usemup(&usem);
}

static void keys_switchToVGA(void) {
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

void keys_enterDebugger(void) {
	keys_switchToVGA();
	debug();
	UIClient::reactivate(UIClient::getActive(),UIClient::getActive(),VGA_MODE);
}

void keys_createTextConsole(void) {
	keys_createConsole(VTERM_PROG,TUI_DEF_COLS,TUI_DEF_ROWS,LOGIN_PROG,"TERM");
}

void keys_createGUIConsole(void) {
	keys_createConsole(WINMNG_PROG,GUI_DEF_RES_X,GUI_DEF_RES_Y,GLOGIN_PROG,"WINMNG");
}
