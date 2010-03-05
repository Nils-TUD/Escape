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

#ifndef FILEIOINTERN_H_
#define FILEIOINTERN_H_

#include <esc/common.h>
#include <sllist.h>

#define BUF_TYPE_STRING		1
#define BUF_TYPE_FILE		2

/* the number of entries in the hash-map */
#define BUF_MAP_SIZE		8
/* the size of the buffers */
#define IN_BUFFER_SIZE		256
#define OUT_BUFFER_SIZE		256

/* format flags */
#define FFL_PADRIGHT		1
#define FFL_FORCESIGN		2
#define FFL_SPACESIGN		4
#define FFL_PRINTBASE		8
#define FFL_PADZEROS		16
#define FFL_CAPHEX			32
#define FFL_SHORT			64
#define FFL_LONG			128
#define FFL_LONGLONG		256

/* prints ' ' or '+' in front of n if positive and depending on the flags */
#define PRINT_SIGNED_PREFIX(count,buf,n,flags) \
	if((n) > 0) { \
		if(((flags) & FFL_FORCESIGN)) { \
			if(bprintc((buf),'+') == IO_EOF) \
				return (count); \
			(count)++; \
		} \
		else if(((flags) & FFL_SPACESIGN)) { \
			if(bprintc((buf),' ') == IO_EOF) \
				return (count); \
			(count)++; \
		} \
	}

/* prints '0', 'X' or 'x' as prefix depending on the flags and base */
#define PRINT_UNSIGNED_PREFIX(count,buf,base,flags) \
	if(((flags) & FFL_PRINTBASE)) { \
		if((base) == 16 || (base) == 8) { \
			if(bprintc((buf),'0') == IO_EOF) \
				return (count); \
			(count)++; \
		} \
		if((base) == 16) { \
			char c = ((flags) & FFL_CAPHEX) ? 'X' : 'x'; \
			if(bprintc((buf),c) == IO_EOF) \
				return (count); \
			(count)++; \
		} \
	}

/* buffer that is used by printf, sprintf and so on */
typedef struct {
	tFD fd;
	u8 type;
	s32 pos;
	s32 length;
	s32 max;
	char *str;
} sBuffer;

/* an io-buffer for a file-descriptor */
typedef struct {
	sBuffer in;
	sBuffer out;
	s32 error;
} sIOBuffer;

char bprintc(sBuffer *buf,char c);
s32 bprints(sBuffer *buf,const char *str);
s32 bprintnpad(sBuffer *buf,s32 n,u8 pad,u16 flags);
s32 bprintn(sBuffer *buf,s32 n);
s32 bprintupad(sBuffer *buf,u32 u,u8 base,u8 pad,u16 flags);
s32 bprintu(sBuffer *buf,u32 u,u8 base,char *hexchars);
s32 bprintulpad(sBuffer *buf,u64 u,u8 base,u8 pad,u16 flags);
s32 bprintul(sBuffer *buf,u64 u,u8 base,char *hexchars);
s32 bprintpad(sBuffer *buf,s32 count,u16 flags);
s32 bprintlpad(sBuffer *buf,s64 n,u8 pad,u16 flags);
s32 bprintl(sBuffer *buf,s64 l);
s32 bprintdblpad(sBuffer *buf,double d,u8 pad,u16 flags,u32 precision);
s32 bprintdbl(sBuffer *buf,double d,u32 precision);
char bscanc(sBuffer *buf);
s32 bscanback(sBuffer *buf,char c);
s32 bscanl(sBuffer *buf,char *line,u32 max);
s32 bscans(sBuffer *buf,char *buffer,u32 max);
s32 vbscanf(sBuffer *buf,const char *fmt,va_list ap);
sIOBuffer *bget(tFile *stream);
sIOBuffer *bcreate(tFD fd,u8 flags);
s32 bflush(sBuffer *buf);

/* buffer for other file-descriptors */
extern sSLList *bufList;

#endif /* FILEIOINTERN_H_ */
