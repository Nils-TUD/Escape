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
#include <sys/proc.h>
#include <sys/tls.h>
#include <stdio.h>
#include <stdlib.h>

#include "../modules.h"

static int otherThread(void *arg);

static size_t a,b;

int mod_tls(A_UNUSED int argc,A_UNUSED char *argv[]) {
	a = tlsadd();
	b = tlsadd();
	if(startthread(otherThread,NULL) < 0)
		error("Unable to start thread");
	for(size_t i = 0; i < 4; i++) {
		tlsset(a,tlsget(a) + 1);
		tlsset(b,tlsget(b) + 1);
		printf("[%d] t1=%lu, t2=%lu\n",gettid(),tlsget(a),tlsget(b));
	}
	return 0;
}

static int otherThread(A_UNUSED void *arg) {
	for(size_t i = 0; i < 4; i++) {
		tlsset(a,tlsget(a) + 1);
		tlsset(b,tlsget(b) + 1);
		printf("[%d] t1=%lu, t2=%lu\n",gettid(),tlsget(a),tlsget(b));
	}
	return 0;
}
