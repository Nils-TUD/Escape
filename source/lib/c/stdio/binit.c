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
#include <assert.h>

bool binit(FILE *f,int fd,uint flags,char *buffer,size_t size,bool dynamic) {
	assert(buffer == NULL || (flags & (IO_READ | IO_WRITE)) != (IO_READ | IO_WRITE));
	assert(buffer == NULL || size != (size_t)-1);
	f->flags = flags;
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
			f->in.buffer = (char*)malloc(size != (size_t)-1 ? size + 1 : IN_BUFFER_SIZE + 1);
			if(f->in.buffer == NULL)
				goto error;
		}
		f->in.pos = 0;
		f->in.max = buffer ? size : 0;
		if(usemcrt(&f->in.usem,1) < 0)
			goto error;
		f->out.dynamic = 0;
	}
	else
		f->in.fd = -1;
	if(flags & IO_WRITE) {
		f->out.fd = fd;
		if(buffer) {
			assert(dynamic == false);
			f->out.buffer = buffer;
		}
		else {
			size_t defsize = size != (size_t)-1 ? size : OUT_BUFFER_SIZE;
			f->out.buffer = (char*)malloc((dynamic ? DYN_BUFFER_SIZE : defsize) + 1);
			if(f->out.buffer == NULL)
				goto errorRd;
		}
		f->out.pos = 0;
		f->out.max = buffer ? size : (dynamic ? DYN_BUFFER_SIZE : OUT_BUFFER_SIZE);
		if(usemcrt(&f->out.usem,1) < 0)
			goto errorRd;
		f->out.dynamic = dynamic;
	}
	else
		f->out.fd = -1;
	return true;

errorRd:
	if(flags & IO_READ)
		usemdestr(&f->in.usem);
error:
	if(!buffer) {
		free(f->out.buffer);
		free(f->in.buffer);
	}
	return false;
}
