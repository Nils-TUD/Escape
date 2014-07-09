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
#include "iobuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void initStdio(void);
static void deinitStdio(void*);

/* for printu() */
const char *hexCharsBig = "0123456789ABCDEF";
const char *hexCharsSmall = "0123456789abcdef";

static FILE stdBufs[3];
FILE *stdin = stdBufs + STDIN_FILENO;
FILE *stdout = stdBufs + STDOUT_FILENO;
FILE *stderr = stdBufs + STDERR_FILENO;
sSLList iostreams;

void initStdio(void) {
	sll_init(&iostreams,malloc,free);
	atexit(deinitStdio);

	if(!binit(stdin,STDIN_FILENO,O_RDONLY,NULL,IN_BUFFER_SIZE,0,false))
		error("Unable to init stdin");
	if(!binit(stdout,STDOUT_FILENO,O_WRONLY,NULL,0,OUT_BUFFER_SIZE,false))
		error("Unable to init stdin");
	if(!binit(stderr,STDERR_FILENO,O_WRONLY,NULL,0,ERR_BUFFER_SIZE,false))
		error("Unable to init stdin");

	stdin->istty = isatty(STDIN_FILENO);
}

static void deinitStdio(A_UNUSED void *dummy) {
	sSLNode *n;
	for(n = sll_begin(&iostreams); n != NULL; n = n->next)
		fclose((FILE*)n->data);
	fflush(stderr);
	fflush(stdout);
}
