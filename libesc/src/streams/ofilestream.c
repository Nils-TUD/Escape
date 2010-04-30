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
#include <esc/lock.h>
#include <esc/dir.h>
#include <esc/io.h>
#include <mem/heap.h>
#include <streams/ofilestream.h>
#include <streams/outputstream.h>
#include <exceptions/io.h>
#include <assert.h>

#define BUF_SIZE	256

static s32 ofstream_writec(sOStream *s,char c);

sOStream *ofstream_open(const char *file,u8 mode) {
	char path[MAX_PATH_LEN];
	tFD fd;
	assert((mode & ~(IO_WRITE | IO_APPEND | IO_CREATE | IO_TRUNCATE)) == 0);
	abspath(path,sizeof(path),file);
	fd = open(path,mode);
	if(fd < 0)
		THROW(IOException,fd);
	return ofstream_openfd(fd);
}

sOStream *ofstream_openfd(tFD fd) {
	sOFStream *s = (sOFStream*)heap_alloc(sizeof(sOFStream));
	sOStream *out = ostream_open();
	out->obj = s;
	out->writec = ofstream_writec;
	out->eof = ofstream_eof;
	out->seek = ofstream_seek;
	out->flush = ofstream_flush;
	out->close = ofstream_close;
	s->fd = fd;
	s->buffer = (char*)heap_alloc(BUF_SIZE);
	s->max = BUF_SIZE;
	s->pos = 0;
	s->lck = 0;
	return out;
}

s32 ofstream_seek(sOStream *s,s32 offset,u32 whence) {
	s32 res;
	sOFStream *fs = (sOFStream*)s->obj;
	/* flush the output before we seek */
	s->flush(s);
	res = seek(fs->fd,offset,whence);
	if(res < 0)
		THROW(IOException,res);
	return res;
}

bool ofstream_eof(sOStream *s) {
	sOFStream *fs = (sOFStream*)s->obj;
	return eof(fs->fd);
}

void ofstream_flush(sOStream *s) {
	sOFStream *fs = (sOFStream*)s->obj;
	if(fs->pos > 0) {
		s32 res;
		locku(&fs->lck);
		if((res = write(fs->fd,fs->buffer,fs->pos * sizeof(char))) < 0)
			THROW(IOException,res);
		fs->pos = 0;
		unlocku(&fs->lck);
	}
}

void ofstream_close(sOStream *s) {
	sOFStream *fs = (sOFStream*)s->obj;
	s->flush(s);
	ostream_close(s);
	close(fs->fd);
	heap_free(fs->buffer);
	heap_free(fs);
}

static s32 ofstream_writec(sOStream *s,char c) {
	sOFStream *fs = (sOFStream*)s->obj;
	if(fs->pos >= fs->max)
		s->flush(s);
	fs->buffer[fs->pos++] = c;
	return 1;
}
