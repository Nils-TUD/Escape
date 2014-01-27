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
#include <esc/io.h>
#include "iobuf.h"
#include <stdio.h>
#include <string.h>

size_t fread(void *ptr,size_t size,size_t count,FILE *file) {
	sIOBuf *buf = &file->in;
	int res = 1; /* for reading from buffer something > 0 */
	size_t rem = size * count;
	char *cptr = (char*)ptr;
	if(buf->fd < 0 || file->eof)
		return 0;
	/* at first, read from the buffer, if there is something left */
	if(buf->pos < buf->max) {
		size_t amount = MIN(rem,(size_t)(buf->max - buf->pos));
		memcpy(cptr,buf->buffer + buf->pos,amount);
		buf->pos += amount;
		cptr += amount;
		rem -= amount;
	}
	/* if its more than the buffer-capacity, better read it at once without buffer */
	if(rem > IN_BUFFER_SIZE) {
		res = IGNSIGS(read(buf->fd,cptr,rem));
		if(res > 0)
			rem -= res;
	}
	/* otherwise fill the buffer and copy the part the user wants */
	else if(rem > 0) {
		res = IGNSIGS(read(buf->fd,buf->buffer,IN_BUFFER_SIZE));
		if(res > 0) {
			size_t amount = MIN((size_t)res,rem);
			memcpy(cptr,file->in.buffer,amount);
			buf->pos = amount;
			buf->max = res;
			rem -= amount;
		}
	}
	/* set eof and error */
	if(res == 0)
		file->eof = true;
	else if(res < 0)
		file->error = res;
	return ((count * size) - rem) / size;
}
