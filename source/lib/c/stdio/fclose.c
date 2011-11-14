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

int fclose(FILE *stream) {
	int res = 0;
	fflush(stream);
	if(stream->in.fd >= 0)
		close(stream->in.fd);
	else if(stream->out.fd >= 0)
		close(stream->out.fd);
	free(stream->in.buffer);
	free(stream->out.buffer);
	if(stream->in.fd >= 0 || stream->out.fd >= 0) {
		if(sll_removeFirstWith(&iostreams,stream) == -1)
			res = -1;
	}
	free(stream);
	return res;
}
