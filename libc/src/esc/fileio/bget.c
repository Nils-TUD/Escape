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
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/proc.h>
#include <esc/heap.h>
#include "fileiointern.h"

static bool registeredExitFunc = false;

static void flushStdStreams(void) {
	fflush(stdout);
	fflush(stderr);
}

sIOBuffer *bget(tFile *stream) {
	sIOBuffer *buf = (sIOBuffer*)stream;
	if(stream == NULL)
		return NULL;

	if(!registeredExitFunc) {
		atexit(flushStdStreams);
		registeredExitFunc = true;
	}

	/* if uninitialized, we have to create it */
	if(buf->in.type == 0 && buf->out.type == 0) {
		buf->error = 0;
		if(buf == stdin) {
			buf->out.fd = -1;
			buf->in.fd = STDIN_FILENO;
			buf->in.type = BUF_TYPE_FILE;
			if(isavterm(buf->in.fd))
				buf->in.type |= BUF_TYPE_VTERM;
			buf->in.pos = 0;
			buf->in.length = 0;
			buf->in.max = IN_BUFFER_SIZE;
			buf->in.str = (char*)malloc(IN_BUFFER_SIZE + 1);
			if(buf->in.str == NULL) {
				free(buf);
				return NULL;
			}
		}
		else {
			buf->in.fd = -1;
			buf->out.fd = buf == stdout ? STDOUT_FILENO : STDERR_FILENO;
			buf->out.type = BUF_TYPE_FILE;
			if(isavterm(buf->out.fd))
				buf->out.type |= BUF_TYPE_VTERM;
			buf->out.pos = 0;
			buf->out.length = 0;
			buf->out.max = OUT_BUFFER_SIZE;
			buf->out.str = (char*)malloc(OUT_BUFFER_SIZE + 1);
			if(buf->out.str == NULL) {
				free(buf);
				return NULL;
			}
		}
	}

	return buf;
}
