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
#include <esc/mem/heap.h>
#include <esc/io/console.h>
#include <esc/io/ofilestream.h>
#include <esc/io/outputstream.h>
#include <esc/exceptions/io.h>
#include <assert.h>

#define BUF_SIZE	512

typedef struct {
	tFD fd;
	s32 pos;
	s32 max;
	char *buffer;
	tULock lck;
} sOFStream;

static tFD ofstream_fileno(sOStream *s);
static s32 ofstream_write(sOStream *s,const void *buffer,u32 count);
static s32 ofstream_seek(sOStream *s,s32 offset,u32 whence);
static bool ofstream_eof(sOStream *s);
static void ofstream_flush(sOStream *s);
static void ofstream_close(sOStream *s);
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
	out->_obj = s;
	out->write = ofstream_write;
	out->writec = ofstream_writec;
	out->eof = ofstream_eof;
	out->fileno = ofstream_fileno;
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

static tFD ofstream_fileno(sOStream *s) {
	sOFStream *fs = (sOFStream*)s;
	return fs->fd;
}

static s32 ofstream_write(sOStream *s,const void *buffer,u32 count) {
	s32 res;
	sOFStream *fs = (sOFStream*)s->_obj;
	/* first flush the output, just to be sure */
	s->flush(s);
	res = write(fs->fd,buffer,count);
	if(res < 0)
		THROW(IOException,res);
	return res;
}

static s32 ofstream_seek(sOStream *s,s32 offset,u32 whence) {
	s32 res;
	sOFStream *fs = (sOFStream*)s->_obj;
	/* flush the output before we seek */
	s->flush(s);
	res = seek(fs->fd,offset,whence);
	if(res < 0)
		THROW(IOException,res);
	return res;
}

static bool ofstream_eof(sOStream *s) {
	UNUSED(s);
	return false;
}

static void ofstream_flush(sOStream *s) {
	sOFStream *fs = (sOFStream*)s->_obj;
	if(fs->pos > 0) {
		s32 res;
		locku(&fs->lck);
		if((res = write(fs->fd,fs->buffer,fs->pos * sizeof(char))) < 0) {
			unlocku(&fs->lck);
			THROW(IOException,res);
		}
		fs->pos = 0;
		unlocku(&fs->lck);
	}
}

static void ofstream_close(sOStream *s) {
	sOFStream *fs = (sOFStream*)s->_obj;
	s->flush(s);
	ostream_close(s);
	close(fs->fd);
	heap_free(fs->buffer);
	heap_free(fs);
}

static s32 ofstream_writec(sOStream *s,char c) {
	sOFStream *fs = (sOFStream*)s->_obj;
	/* ignore '\0' here */
	if(c) {
		if(fs->pos >= fs->max)
			s->flush(s);
		fs->buffer[fs->pos++] = c;
		/* flush stderr on '\n' */
		if((s == cerr || s == cout) && c == '\n')
			s->flush(s);
		return 1;
	}
	return 0;
}
