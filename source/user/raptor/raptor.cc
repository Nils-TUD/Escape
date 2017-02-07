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

#include <sys/common.h>
#include <sys/esccodes.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"

static const int INTERVAL = 5000; /* us */

static volatile bool run = true;

static void sigInt(A_UNUSED int sig) {
	run = false;
}

int main(int argc,char *argv[]) {
	if((argc != 1 && argc != 3) || getopt_ishelp(argc,argv)) {
		fprintf(stderr,"Usage: %s [<cols> <rows>]\n",argv[0]);
		return EXIT_FAILURE;
	}

	uint cols = 100;
	uint rows = 37;
	if(argc == 3) {
		cols = atoi(argv[1]);
		rows = atoi(argv[2]);
	}

	if(signal(SIGINT,sigInt) == SIG_ERR)
		error("Unable to set sig-handler");

	ulong ticks = 0;
	Game game(cols,rows);
	while(run) {
		uint64_t next = rdtsc() + timetotsc(INTERVAL);

		if(!game.tick(ticks))
			break;

		/* wait for next tick */
		uint64_t now = rdtsc();
		if(next > now)
			usleep(tsctotime(next - now));
		ticks++;
	}

	return EXIT_SUCCESS;
}
