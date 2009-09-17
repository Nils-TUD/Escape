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
#include <esc/heap.h>
#include "fileiointern.h"

/* buffer for STDIN, STDOUT and STDERR */
static sIOBuffer stdBufs[3];

sIOBuffer *bget(tFile *stream) {
	tFD fd = -1;
	tFile **stdStr;
	if(stream == NULL)
		return NULL;

	/* check if it is an uninitialized std-stream */
	switch((u32)stream) {
		case STDIN_NOTINIT:
			fd = STDIN_FILENO;
			stdStr = &stdin;
			break;
		case STDOUT_NOTINIT:
			fd = STDOUT_FILENO;
			stdStr = &stdout;
			break;
		case STDERR_NOTINIT:
			fd = STDERR_FILENO;
			stdStr = &stderr;
			break;
	}

	/* if so, we have to create it */
	if(fd != -1) {
		sIOBuffer *buf = stdBufs + fd;
		if(fd == STDIN_FILENO) {
			buf->out.fd = -1;
			buf->in.fd = fd;
			buf->in.type = BUF_TYPE_FILE;
			buf->in.pos = 0;
			buf->in.max = IN_BUFFER_SIZE;
			buf->in.str = (char*)malloc(IN_BUFFER_SIZE + 1);
			if(buf->in.str == NULL) {
				free(buf);
				return NULL;
			}
		}
		else {
			buf->in.fd = -1;
			buf->out.fd = fd;
			buf->out.type = BUF_TYPE_FILE;
			buf->out.pos = 0;
			buf->out.max = OUT_BUFFER_SIZE;
			buf->out.str = (char*)malloc(OUT_BUFFER_SIZE + 1);
			if(buf->out.str == NULL) {
				free(buf);
				return NULL;
			}
		}
		/* store std-stream */
		*stdStr = buf;
		stream = (tFile*)buf;
	}

	return (sIOBuffer*)stream;
}
