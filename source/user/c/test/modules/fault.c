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
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/proc.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "fault.h"

static void sig_segf(int sig) {
	UNUSED(sig);
	fflush(stdout);
	if(setSigHandler(SIG_SEGFAULT,SIG_DFL) < 0)
		error("Unable to unset signal-handler");
	if(sendSignalTo(getpid(),SIG_SEGFAULT) < 0)
		error("Unable to commit suicide");
}

int mod_fault(int argc,char *argv[]) {
	uint *ptr;
	int fd;
	UNUSED(argc);
	UNUSED(argv);
	if(setSigHandler(SIG_SEGFAULT,sig_segf) < 0)
		error("Unable to set signal-handler");
	printf("I am evil ^^\n");
	fd = open((char*)0x12345678,IO_READ);
	ptr = (uint*)0xFFFFFFFF;
	*ptr = 1;
	printf("Never printed\n");
	close(fd);
	return EXIT_SUCCESS;
}
