/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

int bputc(FILE *f,int c) {
	sIOBuf *buf = &f->out;
	if(buf->buffer == NULL)
		return EOF;
	if(buf->fd >= 0) {
		if(buf->pos >= buf->max)
			RETERR(bflush(f));
		buf->buffer[buf->pos++] = c;
		/* flush stderr on '\n' */
		if(f == stderr && c == '\n')
			RETERR(bflush(f));
	}
	else {
		if(buf->pos >= buf->max) {
			char *old;
			if(!buf->dynamic)
				return EOF;

			old = buf->buffer;
			buf->buffer = realloc(buf->buffer,buf->max * 2);
			if(!buf->buffer) {
				buf->buffer = old;
				return EOF;
			}
			buf->max *= 2;
		}
		buf->buffer[buf->pos++] = c;
	}
	/* cast to unsigned char and back to int to ensure that chars < 0 (e.g. german umlaute) are
	 * not negative */
	return (int)(uchar)c;
}
