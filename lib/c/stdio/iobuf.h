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

#ifndef IOBUF_H_
#define IOBUF_H_

#include <esc/common.h>
#include <esc/lock.h>
#include <esc/sllist.h>

#define IN_BUFFER_SIZE		128
#define OUT_BUFFER_SIZE		512
#define ERR_BUFFER_SIZE		64

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

/* execute <expr> and return if its < 0; otherwise you get the value of <expr> */
#define RETERR(expr)		({ \
		s32 __err = (expr); \
		if(__err < 0) \
			return __err; \
		__err; \
	})

/* prints ' ' or '+' in front of n if positive and depending on the flags */
#define PRINT_SIGNED_PREFIX(count,s,n,flags) \
	if((n) > 0) { \
		if(((flags) & FFL_FORCESIGN)) { \
			RETERR(bputc(s,'+')); \
			(count)++; \
		} \
		else if(((flags) & FFL_SPACESIGN)) { \
			RETERR(bputc(s,' ')); \
			(count)++; \
		} \
	}

/* prints '0', 'X' or 'x' as prefix depending on the flags and base */
#define PRINT_UNSIGNED_PREFIX(count,s,base,flags) \
	if(((flags) & FFL_PRINTBASE)) { \
		if((base) == 16 || (base) == 8) { \
			RETERR(bputc(s,'0')); \
			(count)++; \
		} \
		if((base) == 16) { \
			RETERR(bputc(s,((flags) & FFL_CAPHEX) ? 'X' : 'x')); \
			(count)++; \
		} \
	}

typedef struct {
	tFD fd;
	s32 pos;
	s32 max;
	char *buffer;
	tULock lck;
} sIOBuf;

typedef struct {
	u8 eof;
	s32 error;
	sIOBuf in;
	sIOBuf out;
} FILE;

s32 bputc(FILE *f,char c);
s32 bputs(FILE *f,const char *str);
s32 bflush(FILE *f);
s32 bprintpad(FILE *f,s32 count,u16 flags);
s32 bprintn(FILE *f,s32 n);
s32 bprintnpad(FILE *f,s32 n,u8 pad,u16 flags);
s32 bprintl(FILE *f,s64 l);
s32 bprintlpad(FILE *f,s64 n,u8 pad,u16 flags);
s32 bprintu(FILE *f,u32 u,u8 base,const char *hexchars);
s32 bprintupad(FILE *f,u32 u,u8 base,u8 pad,u16 flags);
s32 bprintul(FILE *f,u64 u,u8 base,const char *hexchars);
s32 bprintulpad(FILE *f,u64 u,u8 base,u8 pad,u16 flags);
s32 bprintdbl(FILE *f,double d,u32 precision);
s32 bprintdblpad(FILE *f,double d,u8 pad,u16 flags,u32 precision);
s32 vbprintf(FILE *f,const char *fmt,va_list ap);

s32 bgetc(FILE *f);
char *bgets(FILE *f,char *str,s32 size);
s32 breadn(FILE *f,s32 length,char c);
s32 breads(FILE *f,s32 length,char *str);
s32 vbscanf(FILE *f,const char *fmt,va_list ap);

FILE *bcreate(tFD fd,u8 flags,char *buffer,u32 size);

extern const char *hexCharsBig;
extern const char *hexCharsSmall;
extern sSLList iostreams;

#endif /* IOBUF_H_ */
