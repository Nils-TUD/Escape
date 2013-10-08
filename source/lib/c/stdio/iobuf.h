/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#pragma once

#define IOBUF_H_
#include <esc/common.h>
#include <esc/thread.h>
#include <esc/sllist.h>

#define IN_BUFFER_SIZE		1024
#define OUT_BUFFER_SIZE		1024
#define ERR_BUFFER_SIZE		128
#define DYN_BUFFER_SIZE		128

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
#define FFL_SIZE			512

/* execute <expr> and return if its < 0; otherwise you get the value of <expr> */
#define RETERR(expr)		({ \
		int __err = (expr); \
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
	int fd;
	size_t pos;
	size_t max;
	uchar dynamic;
	char *buffer;
	tULock lck;
} sIOBuf;

typedef struct {
	uint flags;
	uchar eof;
	char istty;	/* only for stdin */
	int error;
	sIOBuf in;
	sIOBuf out;
} FILE;

int bputc(FILE *f,int c);
int bputs(FILE *f,const char *str);
int bflush(FILE *f);
int bprintpad(FILE *f,size_t count,uint flags);
int bprintn(FILE *f,int n);
int bprintnpad(FILE *f,int n,uint pad,uint flags);
int bprintl(FILE *f,llong l);
int bprintlpad(FILE *f,llong n,uint pad,uint flags);
int bprintu(FILE *f,uint u,uint base,const char *hexchars);
int bprintupad(FILE *f,uint u,uint base,uint pad,uint flags);
int bprintul(FILE *f,ullong u,uint base,const char *hexchars);
int bprintulpad(FILE *f,ullong u,uint base,uint pad,uint flags);
int bprintdbl(FILE *f,double d,uint precision);
int bprintdblpad(FILE *f,double d,uint pad,uint flags,uint precision);
int vbprintf(FILE *f,const char *fmt,va_list ap);

int bback(FILE *file);
int bgetc(FILE *f);
char *bgets(FILE *f,char *str,size_t size);
int breadn(FILE *f,int *num,size_t length,int c);
int breads(FILE *f,size_t length,char *str);
int vbscanf(FILE *f,const char *fmt,va_list ap);

FILE *bcreate(int fd,uint flags,char *buffer,size_t size,bool dynamic);
bool binit(FILE *f,int fd,uint flags,char *buffer,size_t size,bool dynamic);

extern const char *hexCharsBig;
extern const char *hexCharsSmall;
extern sSLList iostreams;
