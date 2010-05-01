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

#ifndef OUTPUTSTREAM_H_
#define OUTPUTSTREAM_H_

#include <esc/common.h>
#include <stdarg.h>

typedef struct sOStream sOStream;
typedef s32 (*fWrite)(sOStream *s,const void *buffer,u32 count);
typedef s32 (*fWritec)(sOStream *s,char c);
typedef s32 (*fWrites)(sOStream *s,const char *str);
typedef s32 (*fWriten)(sOStream *s,s32 n);
typedef s32 (*fWritel)(sOStream *s,s64 n);
typedef s32 (*fWriteu)(sOStream *s,u32 u,u8 base);
typedef s32 (*fWriteul)(sOStream *s,u64 u,u8 base);
typedef s32 (*fWritedbl)(sOStream *s,double d,u32 precision);
typedef s32 (*fOutFormat)(sOStream *s,const char *fmt,...);
typedef s32 (*fOutVformat)(sOStream *s,const char *fmt,va_list ap);
typedef s32 (*fOutSeek)(sOStream *s,s32 offset,u32 whence);
typedef bool (*fOutEof)(sOStream *s);
typedef void (*fFlush)(sOStream *s);
typedef void (*fOutClose)(sOStream *s);

struct sOStream {
	fWrite write;
	fWritec writec;
	fWrites writes;
	fWriten writen;
	fWritel writel;
	fWriteu writeu;
	fWriteul writeul;
	fWritedbl writedbl;
	fOutFormat format;
	fOutVformat vformat;
	fOutSeek seek;
	fOutEof eof;
	fFlush flush;
	fOutClose close;
	void *obj;
};

sOStream *ostream_open(void);
void ostream_close(sOStream *s);
s32 ostream_writes(sOStream *s,const char *str);
s32 ostream_writen(sOStream *s,s32 n);
s32 ostream_writel(sOStream *s,s64 l);
s32 ostream_writeu(sOStream *s,u32 u,u8 base);
s32 ostream_writeul(sOStream *s,u64 u,u8 base);
s32 ostream_writedbl(sOStream *s,double d,u32 precision);
s32 ostream_format(sOStream *s,const char *fmt,...);
s32 ostream_vformat(sOStream *s,const char *fmt,va_list ap);

#endif /* OUTPUTSTREAM_H_ */
