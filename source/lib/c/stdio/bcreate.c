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
#include "iobuf.h"
#include <stdio.h>
#include <stdlib.h>

FILE *bcreate(tFD fd,uint flags,char *buffer,size_t size) {
	FILE *f = (FILE*)malloc(sizeof(FILE));
	if(!f)
		return NULL;

	f->eof = false;
	f->error = 0;
	f->istty = 0;
	f->in.buffer = NULL;
	f->out.buffer = NULL;

	if(flags & IO_READ) {
		f->in.fd = fd;
		if(buffer)
			f->in.buffer = buffer;
		else {
			f->in.buffer = (char*)malloc(IN_BUFFER_SIZE + 1);
			if(f->in.buffer == NULL)
				goto error;
		}
		f->in.pos = 0;
		f->in.max = buffer ? size : 0;
		f->in.lck = 0;
	}
	else
		f->in.fd = -1;
	if(flags & IO_WRITE) {
		f->out.fd = fd;
		if(buffer)
			f->out.buffer = buffer;
		else {
			f->out.buffer = (char*)malloc(OUT_BUFFER_SIZE + 1);
			if(f->out.buffer == NULL)
				goto error;
		}
		f->out.pos = 0;
		f->out.max = buffer ? size : OUT_BUFFER_SIZE;
		f->out.lck = 0;
	}
	else
		f->out.fd = -1;
	return f;

error:
	if(!buffer) {
		free(f->out.buffer);
		free(f->in.buffer);
	}
	free(f);
	return NULL;
}
