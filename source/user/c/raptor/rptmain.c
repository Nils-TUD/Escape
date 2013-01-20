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
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/messages.h>
#include <esc/esccodes.h>
#include <esc/io.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include "game.h"

static int kmMngThread(void *arg);
static void sigUsr1(int sig);
static void sigTimer(int sig);
static void qerror(const char *msg,...);
static void quit(void);

static time_t tsc = 0;

int main(void) {
	/* backup screen and stop vterm to read from keyboard */
	vterm_backup(STDOUT_FILENO);
	vterm_setReadline(STDOUT_FILENO,false);

	if(!game_init()) {
		quit();
		exit(EXIT_FAILURE);
	}

	if(signal(SIG_INTRPT_TIMER,sigTimer) == SIG_ERR)
		qerror("Unable to set sig-handler");
	if(startthread(kmMngThread,NULL) < 0)
		qerror("Unable to start thread");

	game_tick(tsc);
	while(1) {
		/* wait until we get a signal */
		wait(EV_NOEVENT,0);
		if(!game_tick(tsc))
			break;
	}

	quit();
	return EXIT_SUCCESS;
}

static int kmMngThread(A_UNUSED void *arg) {
	if(signal(SIG_USR1,sigUsr1) == SIG_ERR)
		error("Unable to set SIG_USR1-handler");
	while(1) {
		int c,cmd,n1,n2,n3;
		if((c = fgetc(stdin)) == EOF) {
			printe("Unable to read from stdin");
			break;
		}
		if(c != '\033')
			continue;
		cmd = freadesc(stdin,&n1,&n2,&n3);
		if(cmd != ESCC_KEYCODE)
			continue;
		game_handleKey(n2,n3,n1);
	}
	return 0;
}

static void sigUsr1(A_UNUSED int sig) {
	exit(EXIT_SUCCESS);
}

static void sigTimer(A_UNUSED int sig) {
	tsc++;
}

static void qerror(const char *msg,...) {
	va_list ap;
	quit();
	va_start(ap,msg);
	vprinte(msg,ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

static void quit(void) {
	game_deinit();
	if(kill(getpid(),SIG_USR1) < 0)
		printe("Unable to send SIG_USR1");
	vterm_setReadline(STDOUT_FILENO,true);
	vterm_restore(STDOUT_FILENO);
}
