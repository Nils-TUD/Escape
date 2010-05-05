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
#include <esc/dir.h>
#include <esc/mem/heap.h>
#include <esc/exceptions/io.h>
#include <esc/io/iofilestream.h>
#include <assert.h>

static void iofstream_close(sIOStream *s);

sIOStream *iofstream_open(const char *path,u8 mode) {
	/* just reading XOR writing is supported! */
	assert((mode & (IO_READ | IO_WRITE)) != (IO_READ | IO_WRITE));
	if(mode & IO_READ)
		return iofstream_linkin(ifstream_open(path,mode));
	return iofstream_linkout(ofstream_open(path,mode));
}

sIOStream *iofstream_openfd(tFD fd,u8 mode) {
	/* just reading XOR writing is supported! */
	assert((mode & (IO_READ | IO_WRITE)) != (IO_READ | IO_WRITE));
	if(mode & IO_READ)
		return iofstream_linkin(ifstream_openfd(fd));
	return iofstream_linkout(ofstream_openfd(fd));
}

sIOStream *iofstream_linkin(sIStream *in) {
	sIOStream *s = (sIOStream*)heap_alloc(sizeof(sIOStream));
	s->in = in;
	s->out = NULL;
	s->_error = 0;
	s->close = iofstream_close;
	return s;
}

sIOStream *iofstream_linkout(sOStream *out) {
	sIOStream *s = (sIOStream*)heap_alloc(sizeof(sIOStream));
	s->in = NULL;
	s->out = out;
	s->_error = 0;
	s->close = iofstream_close;
	return s;
}

static void iofstream_close(sIOStream *s) {
	if(s->in)
		s->in->close(s->in);
	else
		s->out->close(s->out);
	heap_free(s);
}
