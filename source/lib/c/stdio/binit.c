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
#include "iobuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

bool binit(FILE *f,int fd,uint flags,char *buffer,size_t insize,size_t outsize,bool dynamic) {
	assert(buffer == NULL || (flags & (O_RDWR)) != (O_RDWR));
	assert(!(flags & O_WRITE) || outsize > 0);
	f->flags = flags;
	f->eof = false;
	f->error = 0;
	f->istty = 0;
	f->in.buffer = NULL;
	f->out.buffer = NULL;

	if(flags & O_READ) {
		f->in.fd = fd;
		if(buffer)
			f->in.buffer = buffer;
		else {
			f->in.buffer = (char*)malloc(insize + 1);
			if(f->in.buffer == NULL)
				goto error;
		}
		f->in.pos = 0;
		f->in.max = buffer ? insize : 0;
		f->out.dynamic = 0;
	}
	else
		f->in.fd = -1;
	if(flags & O_WRITE) {
		f->out.fd = fd;
		if(buffer) {
			assert(dynamic == false);
			f->out.buffer = buffer;
		}
		else {
			f->out.buffer = (char*)malloc(outsize + 1);
			if(f->out.buffer == NULL)
				goto error;
		}
		f->out.pos = 0;
		f->out.max = outsize;
		f->out.dynamic = dynamic;
	}
	else
		f->out.fd = -1;
	return true;

error:
	if(!buffer) {
		free(f->out.buffer);
		free(f->in.buffer);
	}
	return false;
}
