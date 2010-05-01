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
#include <esc/io.h>
#include <mem/heap.h>
#include <io/streams.h>
#include <io/ifilestream.h>
#include <io/inputstream.h>
#include <exceptions/io.h>
#include <errors.h>
#include <assert.h>

#define BUF_SIZE	64

static s32 ifstream_read(sIStream *s,void *buffer,u32 count);
static s32 ifstream_seek(sIStream *s,s32 offset,u32 whence);
static bool ifstream_eof(sIStream *s);
static void ifstream_close(sIStream *s);
static void ifstream_unread(sIStream *s,char c);
static char ifstream_readc(sIStream *s);

sIStream *ifstream_open(const char *file,u8 mode) {
	char path[MAX_PATH_LEN];
	tFD fd;
	assert((mode & ~IO_READ) == 0);
	abspath(path,sizeof(path),file);
	fd = open(path,mode);
	if(fd < 0)
		THROW(IOException,fd);
	return ifstream_openfd(fd);
}

sIStream *ifstream_openfd(tFD fd) {
	sIFStream *s = (sIFStream*)heap_alloc(sizeof(sIFStream));
	sIStream *in = istream_open();
	in->obj = s;
	in->read = ifstream_read;
	in->readc = ifstream_readc;
	in->unread = ifstream_unread;
	in->eof = ifstream_eof;
	in->seek = ifstream_seek;
	in->close = ifstream_close;
	s->fd = fd;
	s->buffer = (char*)heap_alloc(BUF_SIZE);
	s->length = 0;
	s->pos = 0;
	return in;
}

static s32 ifstream_read(sIStream *s,void *buffer,u32 count) {
	sIFStream *fs = (sIFStream*)s->obj;
	s32 res = read(fs->fd,buffer,count);
	if(res < 0)
		THROW(IOException,res);
	return res;
}

static s32 ifstream_seek(sIStream *s,s32 offset,u32 whence) {
	s32 res;
	sIFStream *fs = (sIFStream*)s->obj;
	res = seek(fs->fd,offset,whence);
	if(res < 0)
		THROW(IOException,res);
	return res;
}

static bool ifstream_eof(sIStream *s) {
	sIFStream *fs = (sIFStream*)s->obj;
	return eof(fs->fd);
}

static void ifstream_close(sIStream *s) {
	sIFStream *fs = (sIFStream*)s->obj;
	istream_close(s);
	close(fs->fd);
	heap_free(fs->buffer);
	heap_free(fs);
}

static void ifstream_unread(sIStream *s,char c) {
	sIFStream *fs = (sIFStream*)s->obj;
	if(fs->pos == 0)
		THROW(IOException,ERR_EOF);
	fs->buffer[fs->pos - 1] = c;
	fs->pos--;
}

static char ifstream_readc(sIStream *s) {
	sIFStream *fs = (sIFStream*)s->obj;
	/* flush stdout if we're stdin */
	if(s == cin)
		cout->flush(cout);
	if(fs->pos >= fs->length) {
		s32 count = read(fs->fd,fs->buffer,BUF_SIZE);
		if(count <= 0)
			THROW(IOException,count == 0 ? ERR_EOF : count);
		fs->pos = 0;
		fs->length = count;
	}
	return fs->buffer[fs->pos++];
}
