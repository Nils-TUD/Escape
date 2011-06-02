/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/mem.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errors.h>
#include <error.h>

#include "sigclone.h"

static tTid parent = -1;
static tTid child = -1;
static size_t parentCount = 0;
static size_t childCount = 0;

static void timerIRQ(int sig) {
	UNUSED(sig);
	if(gettid() == parent)
		parentCount++;
	else
		childCount++;
}

int mod_sigclone(int argc,char *argv[]) {
	int res;
	UNUSED(argc);
	UNUSED(argv);
	parent = gettid();
	if(setSigHandler(SIG_INTRPT_TIMER,timerIRQ) < 0)
		error("Unable to set sighandler");

	if(fork() == 0) {
		child = gettid();
		sleep(20);
		sleep(20);
		sleep(20);
		printf("Child got %d\n",childCount);
		exit(EXIT_SUCCESS);
	}

	/* parent waits */
	do {
		res = waitChild(NULL);
	}
	while(res == ERR_INTERRUPTED);
	printf("Parent got %d\n",parentCount);
	return 0;
}
