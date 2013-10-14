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
#include <esc/driver/screen.h>
#include <esc/keycodes.h>
#include <esc/esccodes.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <stdio.h>
#include <stdlib.h>

#include "keystrokes.h"
#include "clients.h"
#include "jobs.h"
#include "screens.h"

#define MAX_FILEDESCS	16

#define VGA_MODE		3

#define VTERM_PROG		"/sbin/vterm"
#define LOGIN_PROG		"/bin/login"

#define WINMNG_PROG		"/sbin/winmng"
#define GLOGIN_PROG		"/bin/glogin"

#define TUI_DEF_COLS	"100"
#define TUI_DEF_ROWS	"37"

#define GUI_DEF_RES_X	"800"
#define GUI_DEF_RES_Y	"600"

static tULock lck;

void keys_init(void) {
	if(crtlocku(&lck) < 0)
		error("Unable to create keystrokes lock");
}

static void keys_createConsole(const char *mng,const char *cols,const char *rows,const char *login,
							   const char *termVar) {
	char name[SSTRLEN("ui") + 11];
	char path[SSTRLEN("/dev/ui") + 11];

	locku(&lck);
	int id = jobs_getId();
	if(id < 0) {
		fprintf(stderr,"[uimng] Maximum number of clients reached\n");
		unlocku(&lck);
		return;
	}

	snprintf(name,sizeof(name),"ui%d",id);
	snprintf(path,sizeof(path),"/dev/%s",name);

	int mngPid = fork();
	if(mngPid < 0)
		goto error;
	if(mngPid == 0) {
		/* close all but stdin, stdout, stderr */
		for(int i = 3; i < MAX_FILEDESCS; ++i)
			close(i);

		const char *args[] = {mng,cols,rows,name,NULL};
		exec(mng,args);
		printe("exec with %s failed",mng);
		return;
	}

	/* TODO not good */
	while(open(path,IO_MSGS) < 0)
		sleep(20);

	int loginPid = fork();
	if(loginPid < 0)
		goto error;
	if(loginPid == 0) {
		/* close all; login will open different streams */
		for(int i = 0; i < MAX_FILEDESCS; ++i)
			close(i);

		/* set env-var for childs */
		setenv(termVar,path);

		const char *args[] = {login,NULL};
		exec(login,args);
		printe("exec with %s failed",login);
	}

	jobs_add(id,loginPid,mngPid);
	unlocku(&lck);
	return;

error:
	printe("fork failed");
	unlocku(&lck);
}

static void keys_switchToVGA(void) {
	sScreen *scr;
	sScreenMode *mode;
	if(screens_find(VGA_MODE,&mode,&scr)) {
		int fd = open(scr->name,IO_MSGS);
		if(fd < 0) {
			printe("Unable to open '%s'",scr->name);
			return;
		}
		int res = screen_setMode(fd,VID_MODE_TYPE_TUI,VGA_MODE,"",true);
		close(fd);
		if(res < 0)
			printe("Unable to set mode %d",VGA_MODE);
	}
	else
		printe("Unable to find screen for mode %d",VGA_MODE);
}

void keys_enterDebugger(void) {
	keys_switchToVGA();
	debug();
	cli_reactivate(VGA_MODE);
}

void keys_createTextConsole(void) {
	keys_createConsole(VTERM_PROG,TUI_DEF_COLS,TUI_DEF_ROWS,LOGIN_PROG,"TERM");
}

void keys_createGUIConsole(void) {
	keys_createConsole(WINMNG_PROG,GUI_DEF_RES_X,GUI_DEF_RES_Y,GLOGIN_PROG,"WINMNG");
}
