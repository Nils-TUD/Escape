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
#include <esc/io.h>
#include <mem/heap.h>
#include <exceptions/io.h>
#include <io/ostringstream.h>
#include <io/outputstream.h>
#include <errors.h>
#include <string.h>
#include <assert.h>

static s32 osstream_write(sOStream *s,const void *buffer,u32 count);
static s32 osstream_seek(sOStream *s,s32 offset,u32 whence);
static bool osstream_eof(sOStream *s);
static void osstream_flush(sOStream *s);
static void osstream_close(sOStream *s);
static s32 osstream_writec(sOStream *s,char c);

sOStream *osstream_open(char *buffer,s32 size) {
	sOSStream *s = (sOSStream*)heap_alloc(sizeof(sOSStream));
	sOStream *out = ostream_open();
	out->write = osstream_write;
	out->writec = osstream_writec;
	out->eof = osstream_eof;
	out->seek = osstream_seek;
	out->flush = osstream_flush;
	out->close = osstream_close;
	out->obj = s;
	s->buffer = buffer;
	s->max = size <= 0 ? -1 : size;
	s->pos = 0;
	return out;
}

static s32 osstream_write(sOStream *s,const void *buffer,u32 count) {
	sOSStream *ss = (sOSStream*)s->obj;
	s32 amount = MIN(ss->max - ss->pos,(s32)count);
	memcpy(ss->buffer + ss->pos,buffer,amount);
	return amount;
}

static s32 osstream_seek(sOStream *s,s32 offset,u32 whence) {
	sOSStream *ss = (sOSStream*)s->obj;
	switch(whence) {
		case SEEK_CUR:
			offset += ss->pos;
			/* fall through */
		case SEEK_SET:
			if(ss->max != -1 && offset > ss->max)
				ss->pos = ss->max;
			else
				ss->pos = MAX(0,offset);
			break;
		case SEEK_END:
			/* TODO throw exception if max=-1? */
			assert(ss->max != -1);
			ss->pos = ss->max;
			break;
	}
	return ss->pos;
}

static bool osstream_eof(sOStream *s) {
	sOSStream *ss = (sOSStream*)s->obj;
	return ss->max != -1 && ss->pos >= ss->max;
}

static void osstream_flush(sOStream *s) {
	/* ignore */
	UNUSED(s);
}

static void osstream_close(sOStream *s) {
	sOSStream *ss = (sOSStream*)s->obj;
	ostream_close(s);
	heap_free(ss);
}

static s32 osstream_writec(sOStream *s,char c) {
	sOSStream *ss = (sOSStream*)s->obj;
	if(ss->max != -1 && ss->pos >= ss->max)
		THROW(IOException,ERR_EOF);
	ss->buffer[ss->pos++] = c;
	return 1;
}
