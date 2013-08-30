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
#include <esc/thread.h>
#include <stdio.h>
#include <string.h>
#include "highcpu.h"

static int evilThread(A_UNUSED void *arg);

int mod_highcpu(int argc,char *argv[]) {
	size_t i,count = argc > 2 ? atoi(argv[2]) : 1;
	for(i = 0; i < count - 1; i++) {
		if(startthread(evilThread,NULL) < 0)
			printe("Unable to start thread");
	}
	return evilThread(NULL);
}

static int evilThread(A_UNUSED void *arg) {
	volatile int i;
	while(1) {
		for(i = 0; i < 10000000; i++)
			;
		putchar('.');
		fflush(stdout);
	}
	return 0;
}
