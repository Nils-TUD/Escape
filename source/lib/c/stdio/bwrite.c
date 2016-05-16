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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iobuf.h"

ssize_t bwrite(FILE *f,const void *ptr,size_t count) {
	sIOBuf *buf = &f->out;
	if(buf->buffer == NULL)
		return -ENOMEM;
	if(buf->fd >= 0) {
		const char *src = (const char*)ptr;
		size_t rem = count;
		while(rem > 0) {
			size_t amount = MIN(buf->max - buf->pos,rem);
			if(amount > 0) {
				memcpy(buf->buffer + buf->pos,src,amount);
				buf->pos += amount;
				rem -= amount;
				src += amount;
			}
			if(rem > 0)
				RETERR(bflush(f));
		}
	}
	else {
		if(buf->pos + count > buf->max) {
			char *old;
			if(!buf->dynamic)
				return -ENOMEM;

			old = buf->buffer;
			size_t nsize = MAX(buf->max * 2,buf->max + count);
			buf->buffer = realloc(buf->buffer,nsize);
			if(!buf->buffer) {
				buf->buffer = old;
				return -ENOMEM;
			}
			buf->max = nsize;
		}
		memcpy(buf->buffer + buf->pos,ptr,count);
		buf->pos += count;
	}
	return count;
}
