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
#include <esc/mem/heap.h>
#include <esc/io/istringstream.h>
#include <esc/io/inputstream.h>
#include <esc/exceptions/io.h>
#include <errors.h>
#include <string.h>

typedef struct {
	s32 pos;
	s32 length;
	const char *buffer;
} sISStream;

static tFD isstream_fileno(sIStream *s);
static s32 isstream_read(sIStream *s,void *buffer,u32 count);
static s32 isstream_seek(sIStream *s,s32 offset,u32 whence);
static bool isstream_eof(sIStream *s);
static void isstream_close(sIStream *s);
static void isstream_unread(sIStream *s,char c);
static char isstream_getc(sIStream *s);
static char isstream_readc(sIStream *s);

sIStream *isstream_open(const char *str) {
	sISStream *s = (sISStream*)heap_alloc(sizeof(sISStream));
	sIStream *in = istream_open();
	in->_obj = s;
	in->read = isstream_read;
	in->getc = isstream_getc;
	in->readc = isstream_readc;
	in->unread = isstream_unread;
	in->fileno = isstream_fileno;
	/* available and eof is the same here */
	in->available = isstream_eof;
	in->eof = isstream_eof;
	in->seek = isstream_seek;
	in->close = isstream_close;
	s->buffer = str;
	s->length = strlen(str);
	s->pos = 0;
	return in;
}

static tFD isstream_fileno(sIStream *s) {
	UNUSED(s);
	/* no fd here */
	return -1;
}

static s32 isstream_read(sIStream *s,void *buffer,u32 count) {
	sISStream *ss = (sISStream*)s->_obj;
	s32 amount = MIN(ss->length - ss->pos,(s32)count);
	memcpy(buffer,ss->buffer + ss->pos,amount);
	return amount;
}

static s32 isstream_seek(sIStream *s,s32 offset,u32 whence) {
	sISStream *ss = (sISStream*)s->_obj;
	switch(whence) {
		case SEEK_CUR:
			offset += ss->pos;
			/* fall through */
		case SEEK_SET:
			if(offset > ss->length)
				ss->pos = ss->length;
			else
				ss->pos = MAX(0,offset);
			break;
		case SEEK_END:
			ss->pos = ss->length;
			break;
	}
	return ss->pos;
}

static bool isstream_eof(sIStream *s) {
	sISStream *ss = (sISStream*)s->_obj;
	/* null-terminated */
	return ss->pos >= ss->length;
}

static void isstream_close(sIStream *s) {
	sISStream *ss = (sISStream*)s->_obj;
	istream_close(s);
	heap_free(ss);
}

static void isstream_unread(sIStream *s,char c) {
	sISStream *ss = (sISStream*)s->_obj;
	UNUSED(c);
	if(ss->pos == 0)
		THROW(IOException,ERR_EOF);
	/* don't write to the string here */
	ss->pos--;
}

static char isstream_getc(sIStream *s) {
	sISStream *ss = (sISStream*)s->_obj;
	if(ss->pos >= ss->length)
		THROW(IOException,ERR_EOF);
	return ss->buffer[ss->pos];
}

static char isstream_readc(sIStream *s) {
	sISStream *ss = (sISStream*)s->_obj;
	if(ss->pos >= ss->length)
		THROW(IOException,ERR_EOF);
	return ss->buffer[ss->pos++];
}
