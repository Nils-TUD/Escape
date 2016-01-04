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

#include <esc/proto/file.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <sys/sync.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <mutex>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../modules.h"

static std::mutex mutex;
static volatile bool run = true;

static void printffl(const char *fmt,...) {
	std::lock_guard<std::mutex> guard(mutex);
	va_list ap;
	va_start(ap,fmt);
	vprintf(fmt,ap);
	fflush(stdout);
	va_end(ap);
}

static void sig_handler(int) {
	run = false;
}

static int thread_func(A_UNUSED void *arg) {
	if(signal(SIGINT,sig_handler) == SIG_ERR)
		error("Unable to set signal handler");
	while(run)
		printffl("This is the %d test\n",rand());
	return 0;
}

int mod_mutex(int argc,char *argv[]) {
	int clients = 10;
	if(argc > 2)
		clients = atoi(argv[2]);

	srand(rdtsc());
	for(int i = 0; i < clients; i++) {
		if(startthread(thread_func,NULL) < 0)
			error("Unable to start thread");
	}

	IGNSIGS(join(0));
	return EXIT_SUCCESS;
}
