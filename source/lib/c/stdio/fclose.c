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

#include "iobuf.h"

int fclose(FILE *stream) {
	int res = 0;
	fflush(stream);
	if(stream->in.fd >= 0) {
		if((stream->flags & IO_NOCLOSE) == 0)
			close(stream->in.fd);
		free(stream->in.buffer);
	}
	if(stream->out.fd >= 0 || stream->out.dynamic) {
		if(stream->out.fd >= 0 && (stream->flags & IO_NOCLOSE) == 0)
			close(stream->out.fd);
		free(stream->out.buffer);
	}
	/* in the list? (std-streams are not) */
	if(iostreams == stream || stream->prev || stream->next) {
		if(stream->prev)
			stream->prev->next = stream->next;
		else
			iostreams = stream->next;
		if(stream->next)
			stream->next->prev = stream->prev;
	}
	free(stream);
	return res;
}
