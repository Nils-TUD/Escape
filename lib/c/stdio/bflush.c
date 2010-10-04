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

int bflush(FILE *f) {
	sIOBuf *buf = &f->out;
	if(buf->fd >= 0) {
		if(buf->pos > 0) {
			ssize_t res;
			/* flush stdout first if we're stderr */
			if(f == stderr)
				fflush(stdout);
			res = buf->pos;
			buf->pos = 0;
			if((res = write(buf->fd,buf->buffer,res * sizeof(char))) < 0) {
				f->error = (int)res;
				return EOF;
			}
		}
	}
	return 0;
}
