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

s32 bgetc(FILE *f) {
	sIOBuf *buf = &f->in;
	if(f->eof || buf->buffer == NULL)
		return EOF;
	if(buf->fd >= 0) {
		/* flush stdout if we're stdin */
		if(f == stdin)
			fflush(stdout);
		if(buf->pos >= buf->max) {
			s32 count = RETRY(read(buf->fd,buf->buffer,IN_BUFFER_SIZE));
			if(count < 0) {
				f->error = count;
				return EOF;
			}
			if(count == 0) {
				f->eof = true;
				return EOF;
			}
			buf->pos = 0;
			buf->max = count;
		}
	}
	else if(buf->pos >= buf->max)
		return EOF;
	return buf->buffer[buf->pos++];
}
