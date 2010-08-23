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
#include <stdio.h>
#include <stdlib.h>
#include "iobuf.h"

typedef void (*fConstr)(void);
static void initStdio(void);
static void deinitStdio(void*);

fConstr stdioConstr[1] __attribute__((section(".ctors"))) = {
	initStdio
};

/* for printu() */
const char *hexCharsBig = "0123456789ABCDEF";
const char *hexCharsSmall = "0123456789abcdef";

static FILE stdBufs[3];
FILE *stdin = stdBufs + STDIN_FILENO;
FILE *stdout = stdBufs + STDOUT_FILENO;
FILE *stderr = stdBufs + STDERR_FILENO;
sSLList iostreams;

static void initStdio(void) {
	sll_init(&iostreams,malloc,free);
	atexit(deinitStdio);

	stdin->eof = false;
	stdin->error = 0;
	stdin->out.fd = -1;
	stdin->in.fd = STDIN_FILENO;
	stdin->in.buffer = (char*)malloc(IN_BUFFER_SIZE + 1);
	if(stdin->in.buffer == NULL)
		error("Not enough mem for stdin");
	stdin->in.max = 0;
	stdin->in.pos = 0;

	stdout->eof = false;
	stdout->error = 0;
	stdout->in.fd = -1;
	stdout->out.fd = STDOUT_FILENO;
	stdout->out.buffer = (char*)malloc(OUT_BUFFER_SIZE + 1);
	if(stdout->out.buffer == NULL)
		error("Not enough mem for stdout");
	stdout->out.max = OUT_BUFFER_SIZE;
	stdout->out.pos = 0;

	stderr->eof = false;
	stderr->error = 0;
	stderr->in.fd = -1;
	stderr->out.fd = STDERR_FILENO;
	stderr->out.buffer = (char*)malloc(ERR_BUFFER_SIZE + 1);
	if(stderr->out.buffer == NULL)
		error("Not enough mem for stderr");
	stderr->out.max = ERR_BUFFER_SIZE;
	stderr->out.pos = 0;
}

static void deinitStdio(void *dummy) {
	UNUSED(dummy);
	sSLNode *n;
	for(n = sll_begin(&iostreams); n != NULL; n = n->next)
		fclose((FILE*)n->data);
	fflush(stderr);
	fflush(stdout);
}
