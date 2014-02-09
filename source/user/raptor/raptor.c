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
#include <esc/driver/vterm.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/messages.h>
#include <esc/esccodes.h>
#include <esc/io.h>
#include <esc/cmdargs.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include "game.h"

static void sigTimer(int sig);
static void sigInt(int sig);
static void qerror(const char *msg,...);

static time_t tsc = 0;
static volatile bool run = true;

int main(int argc,char *argv[]) {
	if((argc != 1 && argc != 3) || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s [<cols> <rows>]\n",argv[0]);
		return EXIT_FAILURE;
	}

	uint cols = 100;
	uint rows = 37;
	if(argc == 3) {
		cols = atoi(argv[1]);
		rows = atoi(argv[2]);
	}

	game_init(cols,rows);

	if(signal(SIG_INTRPT_TIMER,sigTimer) == SIG_ERR)
		qerror("Unable to set sig-handler");
	if(signal(SIG_INTRPT,sigInt) == SIG_ERR)
		qerror("Unable to set sig-handler");

	game_tick(tsc);
	while(run) {
		/* wait until we get a signal */
		sleep(1000);
		if(!game_tick(tsc))
			break;
	}

	game_deinit();
	return EXIT_SUCCESS;
}

static void sigTimer(A_UNUSED int sig) {
	tsc++;
}

static void sigInt(A_UNUSED int sig) {
	run = false;
}

static void qerror(const char *msg,...) {
	va_list ap;
	game_deinit();
	va_start(ap,msg);
	vprinte(msg,ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}
