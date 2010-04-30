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

#ifndef INPUTSTREAM_H_
#define INPUTSTREAM_H_

#include <esc/common.h>
#include <stdarg.h>

typedef struct sIStream sIStream;
typedef char (*fReadc)(sIStream *s);
typedef s32 (*fReads)(sIStream *s,char *str,u32 size);
typedef s32 (*fReadline)(sIStream *s,char *str,u32 size);
typedef s32 (*fInFormat)(sIStream *s,const char *fmt,...);
typedef s32 (*fInVformat)(sIStream *s,const char *fmt,va_list ap);
typedef void (*fUnread)(sIStream *s,char c);
typedef s32 (*fInSeek)(sIStream *s,s32 offset,u32 whence);
typedef bool (*fInEof)(sIStream *s);
typedef void (*fInClose)(sIStream *s);

struct sIStream {
	fReadc readc;
	fReads reads;
	fReadline readline;
	fInFormat format;
	fInVformat vformat;
	fUnread unread;
	fInSeek seek;
	fInEof eof;
	fInClose close;
	void *obj;
};

sIStream *istream_open(void);
void istream_close(sIStream *s);
s32 istream_reads(sIStream *s,char *str,u32 size);
s32 istream_readline(sIStream *s,char *str,u32 size);
s32 istream_format(sIStream *s,const char *fmt,...);
s32 istream_vformat(sIStream *s,const char *fmt,va_list ap);

#endif /* INPUTSTREAM_H_ */
