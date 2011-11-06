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
#include <stdlib.h>
#include <stdio.h>
#include "maxthreads.h"

static int threadEntry(A_UNUSED void *arg);

int mod_maxthreads(A_UNUSED int argc,A_UNUSED char *argv[]) {
	while(true) {
		if(startThread(threadEntry,NULL) < 0) {
			printe("Unable to start thread");
			break;
		}
	}
	wait(EV_NOEVENT,0);
	return EXIT_SUCCESS;
}

static int threadEntry(A_UNUSED void *arg) {
	wait(EV_NOEVENT,0);
	return 0;
}
