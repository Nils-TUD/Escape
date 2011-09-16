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
#include <esc/io.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <errors.h>
#include "game.h"

static void sigTimer(int sig);
static void qerror(const char *msg,...);
static void quit(void);

static int keymap = -1;
static sKmData kmdata;
static time_t time = 0;

int main(void) {
	/* backup screen and stop vterm to read from keyboard */
	vterm_backup(STDOUT_FILENO);
	vterm_setReadKB(STDOUT_FILENO,false);

	keymap = open("/dev/kmmanager",IO_READ | IO_NOBLOCK);
	if(keymap < 0)
		qerror("Unable to open keymap-driver");

	if(!game_init()) {
		quit();
		exit(EXIT_FAILURE);
	}

	if(setSigHandler(SIG_INTRPT_TIMER,sigTimer) < 0)
		qerror("Unable to set sig-handler");

	game_tick(time);
	while(1) {
		if(wait(EV_DATA_READABLE,keymap) != ERR_INTERRUPTED) {
			ssize_t res = RETRY(read(keymap,&kmdata,sizeof(kmdata)));
			if(res < 0) {
				if(res != ERR_WOULD_BLOCK)
					qerror("Unable to read from keymap: %d",res);
			}
			else if(res > 0)
				game_handleKey(kmdata.keycode,kmdata.modifier,kmdata.isBreak,kmdata.character);
		}

		if(!game_tick(time))
			break;
	}

	quit();
	return EXIT_SUCCESS;
}

static void sigTimer(A_UNUSED int sig) {
	time++;
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
	close(keymap);
	vterm_setReadKB(STDOUT_FILENO,true);
	vterm_restore(STDOUT_FILENO);
}
