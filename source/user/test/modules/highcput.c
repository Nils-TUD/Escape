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
#include <sys/thread.h>
#include <stdio.h>
#include <string.h>

#include "../modules.h"

static int evilThread(void *arg);

int mod_highcput(A_UNUSED int argc,A_UNUSED char *argv[]) {
	while(1) {
		int tid = startthread(evilThread,NULL);
		if(tid < 0)
			printe("Unable to start thread");
		//join(tid);
	}
	return 0;
}

static int evilThread(A_UNUSED void *arg) {
	volatile int i,j;
	for(j = 0; j < 10; ++j) {
		for(i = 0; i < 10000000; i++)
			;
		putchar('.');
		fflush(stdout);
	}
	return 0;
}
