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

int fseek(FILE *stream,int offset,uint whence) {
	int res;
	if(stream->in.fd >= 0) {
		/* if we want to move relatively, we have to reduce the offset by the number of chars left
		 * in buffer. */
		if(whence == SEEK_CUR && stream->in.pos < stream->in.max)
			offset -= stream->in.max - stream->in.pos;
		/* clear buffer */
		stream->in.pos = stream->in.max;
		/* if there is an output-stream, flush it */
		if(stream->out.fd >= 0)
			bflush(stream);
		res = seek(stream->in.fd,offset,whence);
	}
	else {
		/* first flush the buffer */
		bflush(stream);
		res = seek(stream->out.fd,offset,whence);
	}
	if(res < 0) {
		stream->error = res;
		return res;
	}
	return 0;
}
