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

#define MAX_FILEDESCS	16
#define VTERM_PROG		"/sbin/vterm"
#define LOGIN_PROG		"/bin/login"

static tULock lck;

void keys_init(void) {
	if(crtlocku(&lck) < 0)
		error("Unable to create keystrokes lock");
}

void keys_createTextConsole(void) {
	char name[SSTRLEN("vterm") + 11];
	char path[SSTRLEN("/dev/vterm") + 11];

	locku(&lck);
	int id = jobs_getId();
	snprintf(name,sizeof(name),"vterm%d",id);
	snprintf(path,sizeof(path),"/dev/%s",name);

	int vtPid = fork();
	if(vtPid < 0)
		goto error;
	if(vtPid == 0) {
		/* close all but stdin, stdout, stderr */
		for(int i = 3; i < MAX_FILEDESCS; ++i)
			close(i);

		const char *args[] = {VTERM_PROG,"100","37",name,NULL};
		exec(VTERM_PROG,args);
		printe("[UIM] exec with vterm failed");
		return;
	}

	/* TODO not good */
	while(open(path,IO_MSGS) < 0)
		sleep(20);

	int shPid = fork();
	if(shPid < 0)
		goto error;
	if(shPid == 0) {
		/* close all; login will open different streams */
		for(int i = 0; i < MAX_FILEDESCS; ++i)
			close(i);

		const char *args[] = {LOGIN_PROG,path,NULL};
		exec(LOGIN_PROG,args);
		printe("[UIM] exec with login failed");
	}

	jobs_add(id,shPid,vtPid);
	unlocku(&lck);
	return;

error:
	printe("[UIM] fork failed");
	unlocku(&lck);
}

bool keys_handleKey(sKmData *data) {
	if(data->d.keyb.modifier & STATE_BREAK)
		return false;

	if(data->d.keyb.keycode == VK_F12) {
		/* TODO switch video mode */
		debug();
		return true;
	}

	if(!(data->d.keyb.modifier & STATE_CTRL))
		return false;

	switch(data->d.keyb.keycode) {
		case VK_T:
			keys_createTextConsole();
			return true;
		case VK_G:
			return true;
		case VK_LEFT:
			cli_prev();
			return true;
		case VK_RIGHT:
			cli_next();
			return true;
	}
	return false;
}
