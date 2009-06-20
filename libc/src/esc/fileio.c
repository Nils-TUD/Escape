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
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/proc.h>
#include <esc/keycodes.h>
#include <esc/env.h>
#include <esc/heap.h>
#include <string.h>
#include <errors.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <sllist.h>

/* the number of entries in the hash-map */
#define BUF_MAP_SIZE		8
/* the size of the buffers */
#define IN_BUFFER_SIZE		10
#define OUT_BUFFER_SIZE		256

#define BUF_TYPE_STRING		0
#define BUF_TYPE_FILE		1

/* initial values for stdin, stdout and stderr, so that we know that they are not initialized */
#define STDIN_NOTINIT		0xFFFF0001
#define STDOUT_NOTINIT		0xFFFF0002
#define STDERR_NOTINIT		0xFFFF0003

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
			if(doFprintc((buf),'+') == IO_EOF) \
				return (count); \
			(count)++; \
		} \
		else if(((flags) & FFL_SPACESIGN)) { \
			if(doFprintc((buf),' ') == IO_EOF) \
				return (count); \
			(count)++; \
		} \
	}

/* prints '0', 'X' or 'x' as prefix depending on the flags and base */
#define PRINT_UNSIGNED_PREFIX(count,buf,base,flags) \
	if(((flags) & FFL_PRINTBASE)) { \
		if((base) == 16 || (base) == 8) { \
			if(doFprintc((buf),'0') == IO_EOF) \
				return (count); \
			(count)++; \
		} \
		if((base) == 16) { \
			char c = ((flags) & FFL_CAPHEX) ? 'X' : 'x'; \
			if(doFprintc((buf),c) == IO_EOF) \
				return (count); \
			(count)++; \
		} \
	}

/* buffer that is used by printf, sprintf and so on */
typedef struct {
	tFD fd;
	u8 type;
	s32 pos;
	s32 max;
	char *str;
} sBuffer;

/* an io-buffer for a file-descriptor */
typedef struct {
	sBuffer in;
	sBuffer out;
} sIOBuffer;

static char doFprintc(sBuffer *buf,char c);
static s32 doFprints(sBuffer *buf,const char *str);
static s32 doFprintnPad(sBuffer *buf,s32 n,u8 pad,u16 flags);
static s32 doFprintn(sBuffer *buf,s32 n);
static s32 doFprintuPad(sBuffer *buf,u32 u,u8 base,u8 pad,u16 flags);
static s32 doFprintu(sBuffer *buf,u32 u,u8 base,char *hexchars);
static s32 doFprintulPad(sBuffer *buf,u64 u,u8 base,u8 pad,u16 flags);
static s32 doFprintul(sBuffer *buf,u64 u,u8 base,char *hexchars);
static s32 doPad(sBuffer *buf,s32 count,u16 flags);
static s32 doFprintlPad(sBuffer *buf,s64 n,u8 pad,u16 flags);
static s32 doFprintl(sBuffer *buf,s64 l);
static s32 doFprintIEEE754Pad(sBuffer *buf,double d,u8 pad,u16 flags,u32 precision);
static s32 doFprintIEEE754(sBuffer *buf,double d,u32 precision);
static char doFscanc(sBuffer *buf);
static s32 doFscanback(sBuffer *buf,char c);
static s32 doFscanl(sBuffer *buf,char *line,u32 max);
static s32 doFscans(sBuffer *buf,char *buffer,u32 max);
static s32 doVfscanf(sBuffer *buf,const char *fmt,va_list ap);
static sIOBuffer *getBuf(tFile *stream);
static sIOBuffer *createBuffer(tFD fd,u8 flags);
static s32 doFlush(sBuffer *buf);

/* std-streams */
tFile *stdin = (tFile*)STDIN_NOTINIT;
tFile *stdout = (tFile*)STDOUT_NOTINIT;
tFile *stderr = (tFile*)STDERR_NOTINIT;

/* for printu() */
static char hexCharsBig[] = "0123456789ABCDEF";
static char hexCharsSmall[] = "0123456789abcdef";

/* buffer for STDIN, STDOUT and STDERR */
static sIOBuffer stdBufs[3];
/* buffer for other file-descriptors */
static sSLList *bufList = NULL;

tFile *fopen(const char *filename,const char *mode) {
	char c;
	sIOBuffer *buf;
	u8 flags = 0;
	tFD fd;

	/* parse mode */
	c = *mode++;
	switch(c) {
		case 'r':
			flags |= IO_READ;
			break;
		case 'w':
			flags |= IO_WRITE;
			break;
		case 'a':
			/* TODO */
			break;
	}
	/* TODO */
	if(*mode && *mode == '+')
		flags |= 0;

	/* open the file */
	fd = open(filename,flags);
	if(fd < 0)
		return NULL;

	/* create buffer for it */
	buf = createBuffer(fd,flags);
	return (tFile*)buf;
}

u32 fread(void *ptr,u32 size,u32 count,tFile *file) {
	u32 total;
	s32 res;
	u8 *bPtr = (u8*)ptr;
	sBuffer *in;
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	/* no in-buffer? */
	if(buf->in.fd == -1)
		return IO_EOF;

	/* first read from buffer */
	in = &(buf->in);
	total = size * count;
	while(in->pos > 0) {
		*bPtr++ = in->str[--(in->pos)];
		total--;
	}

	/* TODO maybe we should use in smaller steps, if usefull? */
	/* read from file */
	res = read(in->fd,bPtr,total);
	if(res < 0)
		return count - ((total + size - 1) / size);
	/* handle EOF from vterm */
	/* TODO this is not really nice, right? */
	if(res > 0 && bPtr[0] == (u8)IO_EOF)
		return 0;
	total -= res;
	return count - ((total + size - 1) / size);
}

u32 fwrite(const void *ptr,u32 size,u32 count,tFile *file) {
	u32 total;
	u8 *bPtr = (u8*)ptr;
	sBuffer *out;
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	/* no out-buffer? */
	if(buf->out.fd == -1)
		return IO_EOF;

	/* TODO don't write it byte for byte */
	/* write to buffer */
	out = &(buf->out);
	total = size * count;
	while(total > 0) {
		if(doFprintc(out,*bPtr++) == IO_EOF)
			return count - ((total + size - 1) / size);
		total--;
	}
	return count;
}

s32 fflush(tFile *file) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	return doFlush(&(buf->out));
}

s32 fclose(tFile *file) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;

	/* flush the buffer */
	fflush(file);

	/* close file */
	if(buf->in.fd >= 0)
		close(buf->in.fd);
	else
		close(buf->out.fd);

	/* remove and free buffer */
	sll_removeFirst(bufList,buf);
	free(buf->in.str);
	free(buf->out.str);
	free(buf);
	return 0;
}

s32 printe(const char *prefix,...) {
	s32 res = 0;
	char dummyBuf;
	char *msg;
	va_list ap;

	msg = strerror(errno);
	/* if we have no terminal we write it via debugf */
	if(getEnv(&dummyBuf,1,"TERM") == false) {
		va_start(ap,prefix);
		vdebugf(prefix,ap);
		va_end(ap);
		debugf(": %s\n",msg);
		res = 0;
	}
	else {
		va_start(ap,prefix);
		vfprintf(stderr,prefix,ap);
		va_end(ap);
		fprintf(stderr,": %s\n",msg);
	}
	return res;
}

s32 printf(const char *fmt,...) {
	s32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vfprintf(stdout,fmt,ap);
	va_end(ap);
	return count;
}

s32 fprintf(tFile *file,const char *fmt,...) {
	s32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vfprintf(file,fmt,ap);
	va_end(ap);
	return count;
}

s32 sprintf(char *str,const char *fmt,...) {
	s32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vsnprintf(str,-1,fmt,ap);
	va_end(ap);
	return count;
}

s32 snprintf(char *str,u32 max,const char *fmt,...) {
	s32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vsnprintf(str,max,fmt,ap);
	va_end(ap);
	return count;
}

s32 vprintf(const char *fmt,va_list ap) {
	return vfprintf(stdout,fmt,ap);
}

s32 vfprintf(tFile *file,const char *fmt,va_list ap) {
	s32 res;
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	res = doVfprintf(&(buf->out),fmt,ap);
	doFlush(&(buf->out));
	return res;
}

s32 vsprintf(char *str,const char *fmt,va_list ap) {
	return vsnprintf(str,-1,fmt,ap);
}

s32 vsnprintf(char *str,u32 max,const char *fmt,va_list ap) {
	sBuffer buf = {
		.type = BUF_TYPE_STRING,
		.str = str,
		.pos = 0,
		.max = max
	};
	s32 res = doVfprintf(&buf,fmt,ap);
	/* terminate */
	buf.str[buf.pos] = '\0';
	return res;
}

char printc(char c) {
	return fprintc(stdout,c);
}

char fprintc(tFile *file,char c) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	return doFprintc(&(buf->out),c);
}

s32 prints(const char *str) {
	return fprints(stdout,str);
}

s32 fprints(tFile *file,const char *str) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	return doFprints(&(buf->out),str);
}

s32 printn(s32 n) {
	return fprintn(stdout,n);
}

s32 fprintn(tFile *file,s32 n) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	return doFprintn(&(buf->out),n);
}

s32 printu(s32 u,u8 base) {
	return fprintu(stdout,u,base);
}

s32 fprintu(tFile *file,s32 u,u8 base) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	return doFprintu(&(buf->out),u,base,0);
}

void flush(void) {
	fflush(stdout);
}

char scanc(void) {
	return fscanc(stdin);
}

char fscanc(tFile *file) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	return doFscanc(&(buf->in));
}

s32 scanback(char c) {
	return fscanback(stdin,c);
}

s32 fscanback(tFile *file,char c) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return IO_EOF;
	return doFscanback(&(buf->in),c);
}

s32 scanl(char *line,u32 max) {
	return fscanl(stdin,line,max);
}

s32 fscanl(tFile *file,char *line,u32 max) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return 0;
	return doFscanl(&(buf->in),line,max);
}

s32 scans(char *buffer,u32 max) {
	return fscans(stdin,buffer,max);
}

s32 fscans(tFile *file,char *buffer,u32 max) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return 0;
	return doFscans(&(buf->in),buffer,max);
}

s32 scanf(const char *fmt,...) {
	s32 res;
	va_list ap;
	va_start(ap,fmt);
	res = vfscanf(stdin,fmt,ap);
	va_end(ap);
	return res;
}

s32 fscanf(tFile *file,const char *fmt,...) {
	s32 res;
	va_list ap;
	va_start(ap,fmt);
	res = vfscanf(file,fmt,ap);
	va_end(ap);
	return res;
}

s32 sscanf(const char *str,const char *fmt,...) {
	s32 res;
	va_list ap;
	va_start(ap,fmt);
	res = vsscanf(str,fmt,ap);
	va_end(ap);
	return res;
}

s32 vscanf(const char *fmt,va_list ap) {
	return vfscanf(stdin,fmt,ap);
}

s32 vfscanf(tFile *file,const char *fmt,va_list ap) {
	sIOBuffer *buf = getBuf(file);
	if(buf == NULL)
		return 0;
	return doVfscanf(&(buf->in),fmt,ap);
}

s32 vsscanf(const char *str,const char *fmt,va_list ap) {
	sBuffer buf = {
		.type = BUF_TYPE_STRING,
		.pos = 0,
		.max = -1,
		.str = (char*)str
	};
	return doVfscanf(&buf,fmt,ap);
}

u8 getnwidth(s32 n) {
	/* we have at least one char */
	u8 width = 1;
	if(n < 0) {
		width++;
		n = -n;
	}
	while(n >= 10) {
		n /= 10;
		width++;
	}
	return width;
}

u8 getlwidth(s64 l) {
	u8 c = 0;
	if(l < 0) {
		l = -l;
		c++;
	}
	while(l >= 10) {
		c++;
		l /= 10;
	}
	return c + 1;
}

u8 getuwidth(u32 n,u8 base) {
	u8 width = 1;
	while(n >= base) {
		n /= base;
		width++;
	}
	return width;
}

u8 getulwidth(u64 n,u8 base) {
	u8 width = 1;
	while(n >= base) {
		n /= base;
		width++;
	}
	return width;
}

static char doFprintc(sBuffer *buf,char c) {
	if(buf->max != -1 && buf->pos >= buf->max) {
		if(buf->type == BUF_TYPE_FILE) {
			if(doFlush(buf) != 0)
				return IO_EOF;
		}
		else
			return IO_EOF;
	}
	buf->str[buf->pos++] = c;
	return c;
}

static s32 doFprints(sBuffer *buf,const char *str) {
	char c;
	char *start = (char*)str;
	while((c = *str)) {
		/* handle escape */
		if(c == '\033') {
			if(doFprintc(buf,c) == IO_EOF)
				break;
			if(doFprintc(buf,*++str) == IO_EOF)
				break;
			if(doFprintc(buf,*++str) == IO_EOF)
				break;
		}
		else {
			if(doFprintc(buf,c) == IO_EOF)
				break;
		}
		str++;
	}
	return str - start;
}

static s32 doFprintnPad(sBuffer *buf,s32 n,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		u32 width = getnwidth(n);
		if(n > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		count += doPad(buf,pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	PRINT_SIGNED_PREFIX(count,buf,n,flags);
	/* print number */
	count += doFprintn(buf,n);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += doPad(buf,pad - count,flags);
	return count;
}

static s32 doFprintn(sBuffer *buf,s32 n) {
	u32 c = 0;
	if(n < 0) {
		if(doFprintc(buf,'-') == IO_EOF)
			return c;
		n = -n;
		c++;
	}

	if(n >= 10) {
		c += doFprintn(buf,n / 10);
	}
	if(doFprintc(buf,'0' + n % 10) == IO_EOF)
		return c;
	return c + 1;
}

static s32 doFprintuPad(sBuffer *buf,u32 u,u8 base,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getuwidth(u,base);
		count += doPad(buf,pad - width,flags);
	}
	/* print base-prefix */
	PRINT_UNSIGNED_PREFIX(count,buf,base,flags);
	/* pad left - zeros */
	if(!(flags & FFL_PADRIGHT) && (flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getuwidth(u,base);
		count += doPad(buf,pad - width,flags);
	}
	/* print number */
	if(flags & FFL_CAPHEX)
		count += doFprintu(buf,u,base,hexCharsBig);
	else
		count += doFprintu(buf,u,base,hexCharsSmall);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += doPad(buf,pad - count,flags);
	return count;
}

static s32 doFprintu(sBuffer *buf,u32 u,u8 base,char *hexchars) {
	s32 c = 0;
	if(u >= base)
		c += doFprintu(buf,u / base,base,hexchars);
	if(doFprintc(buf,hexchars[(u % base)]) == IO_EOF)
		return c;
	return c + 1;
}

static s32 doFprintulPad(sBuffer *buf,u64 u,u8 base,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getulwidth(u,base);
		count += doPad(buf,pad - width,flags);
	}
	/* print base-prefix */
	PRINT_UNSIGNED_PREFIX(count,buf,base,flags);
	/* pad left - zeros */
	if(!(flags & FFL_PADRIGHT) && (flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getulwidth(u,base);
		count += doPad(buf,pad - width,flags);
	}
	/* print number */
	if(flags & FFL_CAPHEX)
		count += doFprintul(buf,u,base,hexCharsBig);
	else
		count += doFprintul(buf,u,base,hexCharsSmall);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += doPad(buf,pad - count,flags);
	return count;
}

static s32 doFprintul(sBuffer *buf,u64 u,u8 base,char *hexchars) {
	s32 c = 0;
	if(u >= base)
		c += doFprintul(buf,u / base,base,hexchars);
	if(doFprintc(buf,hexchars[(u % base)]) == IO_EOF)
		return c;
	return c + 1;
}

static s32 doFprintlPad(sBuffer *buf,s64 n,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		u32 width = getlwidth(n);
		if(n > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		count += doPad(buf,pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	PRINT_SIGNED_PREFIX(count,buf,n,flags);
	/* print number */
	count += doFprintl(buf,n);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += doPad(buf,pad - count,flags);
	return count;
}

static s32 doFprintl(sBuffer *buf,s64 l) {
	s32 c = 0;
	if(l < 0) {
		if(doFprintc(buf,'-') == IO_EOF)
			return c;
		l = -l;
		c++;
	}
	if(l >= 10)
		c += doFprintl(buf,l / 10);
	if(doFprintc(buf,(l % 10) + '0') == IO_EOF)
		return c;
	return c + 1;
}

static s32 doFprintIEEE754Pad(sBuffer *buf,double d,u8 pad,u16 flags,u32 precision) {
	s32 count = 0;
	s64 pre = (s64)d;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		u32 width = getlwidth(pre) + precision + 1;
		if(pre > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		count += doPad(buf,pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	PRINT_SIGNED_PREFIX(count,buf,pre,flags);
	/* print number */
	count += doFprintIEEE754(buf,d,precision);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		count += doPad(buf,pad - count,flags);
	return count;
}

static s32 doFprintIEEE754(sBuffer *buf,double d,u32 precision) {
	s32 c = 0;
	s64 val = 0;

	/* Note: this is probably not a really good way of converting IEEE754-floating point numbers
	 * to string. But its simple and should be enough for my purposes :) */

	val = (s64)d;
	c += doFprintl(buf,val);
	d -= val;
	if(d < 0)
		d = -d;
	if(doFprintc(buf,'.') == IO_EOF)
		return c;
	c++;
	while(precision-- > 0) {
		d *= 10;
		val = (s64)d;
		if(doFprintc(buf,(val % 10) + '0') == IO_EOF)
			return c;
		d -= val;
		c++;
	}
	return c;
}

static s32 doPad(sBuffer *buf,s32 count,u16 flags) {
	s32 x = 0;
	char c = flags & FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0) {
		if(doFprintc(buf,c) == IO_EOF)
			return x;
		x++;
	}
	return x;
}

s32 doVfprintf(void *vbuf,const char *fmt,va_list ap) {
	sBuffer *buf = (sBuffer*)vbuf;
	char c,b,pad;
	char *s;
	s32 *ptr;
	s32 n;
	u32 u;
	s64 l;
	u64 ul;
	double d;
	float f;
	bool readFlags;
	u16 flags;
	u16 precision;
	u8 width,base;
	s32 count = 0;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* handle escape */
			if(c == '\033') {
				if(doFprintc(buf,c) == IO_EOF)
					return count;
				count++;
				if(doFprintc(buf,*fmt++) == IO_EOF)
					return count;
				count++;
				if(doFprintc(buf,*fmt++) == IO_EOF)
					return count;
				count++;
				continue;
			}

			/* finished? */
			if(c == '\0')
				return count;
			if(doFprintc(buf,c) == IO_EOF)
				return count;
			count++;
		}

		/* read flags */
		flags = 0;
		pad = 0;
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '-':
					flags |= FFL_PADRIGHT;
					fmt++;
					break;
				case '+':
					flags |= FFL_FORCESIGN;
					fmt++;
					break;
				case ' ':
					flags |= FFL_SPACESIGN;
					fmt++;
					break;
				case '#':
					flags |= FFL_PRINTBASE;
					fmt++;
					break;
				case '0':
					flags |= FFL_PADZEROS;
					fmt++;
					break;
				case '*':
					pad = (u8)va_arg(ap, u32);
					fmt++;
					break;
				default:
					readFlags = false;
					break;
			}
		}

		/* read pad-width */
		if(pad == 0) {
			while(*fmt >= '0' && *fmt <= '9') {
				pad = pad * 10 + (*fmt - '0');
				fmt++;
			}
		}

		/* read precision */
		precision = 6;
		if(*fmt == '.') {
			fmt++;
			precision = 0;
			while(*fmt >= '0' && *fmt <= '9') {
				precision = precision * 10 + (*fmt - '0');
				fmt++;
			}
		}

		/* read length */
		switch(*fmt) {
			case 'l':
				flags |= FFL_LONG;
				fmt++;
				break;
			case 'L':
				flags |= FFL_LONGLONG;
				fmt++;
				break;
			case 'h':
				flags |= FFL_SHORT;
				fmt++;
				break;
		}

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
			case 'i':
				if(flags & FFL_LONGLONG) {
					l = va_arg(ap, u32);
					l |= ((s64)va_arg(ap, s32)) << 32;
					count += doFprintlPad(buf,l,pad,flags);
				}
				else {
					n = va_arg(ap, s32);
					if(flags & FFL_SHORT)
						n &= 0xFFFF;
					count += doFprintnPad(buf,n,pad,flags);
				}
				break;

			/* pointer */
			case 'p':
				u = va_arg(ap, u32);
				flags |= FFL_PADZEROS;
				pad = 9;
				count += doFprintuPad(buf,u >> 16,16,pad - 5,flags);
				if(doFprintc(buf,':') == IO_EOF)
					return count;
				count++;
				count += doFprintuPad(buf,u & 0xFFFF,16,4,flags);
				break;

			/* number of chars written so far */
			case 'n':
				ptr = va_arg(ap, s32*);
				*ptr = count;
				break;

			/* floating points */
			case 'f':
				if(flags & FFL_LONG) {
					d = va_arg(ap, double);
					count += doFprintIEEE754Pad(buf,d,pad,flags,precision);
				}
				else {
					/* 'float' is promoted to 'double' when passed through '...' */
					f = va_arg(ap, double);
					count += doFprintIEEE754Pad(buf,f,pad,flags,precision);
				}
				break;

			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				base = c == 'o' ? 8 : ((c == 'x' || c == 'X') ? 16 : (c == 'b' ? 2 : 10));
				if(c == 'X')
					flags |= FFL_CAPHEX;
				if(flags & FFL_LONGLONG) {
					ul = va_arg(ap, u32);
					ul |= ((u64)va_arg(ap, u32)) << 32;
					count += doFprintulPad(buf,ul,base,pad,flags);
				}
				else {
					u = va_arg(ap, u32);
					if(flags & FFL_SHORT)
						u &= 0xFFFF;
					count += doFprintuPad(buf,u,base,pad,flags);
				}
				break;

			/* string */
			case 's':
				s = va_arg(ap, char*);
				if(pad > 0 && !(flags & FFL_PADRIGHT)) {
					width = strlen(s);
					count += doPad(buf,pad - width,flags);
				}
				n = doFprints(buf,s);
				if(pad > 0 && (flags & FFL_PADRIGHT))
					count += doPad(buf,pad - n,flags);
				count += n;
				break;

			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				if(doFprintc(buf,b) == IO_EOF)
					return count;
				count++;
				break;

			default:
				if(doFprintc(buf,c) == IO_EOF)
					return count;
				count++;
				break;
		}
	}
}

static char doFscanc(sBuffer *buf) {
	/* file */
	if(buf->type == BUF_TYPE_FILE) {
		char c;
		if(buf->pos > 0)
			return buf->str[--(buf->pos)];

		if(read(buf->fd,&c,sizeof(char)) <= 0)
			return IO_EOF;
		return c;
	}

	/* string */
	if(buf->str[buf->pos] == '\0')
		return IO_EOF;
	return buf->str[(buf->pos)++];
}

static s32 doFscanback(sBuffer *buf,char c) {
	if(buf->type == BUF_TYPE_FILE) {
		if(buf->pos >= IN_BUFFER_SIZE)
			return IO_EOF;
		buf->str[(buf->pos)++] = c;
		return 0;
	}
	else if(buf->pos > 0) {
		buf->str[--(buf->pos)] = c;
		return 0;
	}
	return IO_EOF;
}

static s32 doFscanl(sBuffer *buf,char *line,u32 max) {
	char c;
	char *start = line;
	/* wait for one char left (\0) or a newline or EOF */
	while(max-- > 1 && (c = doFscanc(buf)) != IO_EOF && c != '\n')
		*line++ = c;
	/* terminate */
	*line = '\0';
	return line - start;
}

static s32 doFscans(sBuffer *buf,char *buffer,u32 max) {
	char c;
	char *start = buffer;
	/* wait for one char left (\0) or till EOF */
	while(max-- > 1 && (c = doFscanc(buf)) != IO_EOF)
		*buffer++ = c;
	/* terminate */
	*buffer = '\0';
	return buffer - start;
}

static s32 doVfscanf(sBuffer *buf,const char *fmt,va_list ap) {
	char *s = NULL,c,tc,rc = 0,length;
	const char *numTable = "0123456789abcdef";
	s32 *n,count = 0;
	u32 *u,x;
	u8 base;
	bool readFlags;
	bool shortPtr;
	bool discard;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			if(c != (rc = doFscanc(buf)) || rc == IO_EOF)
				return count;
			/* finished? */
			if(c == '\0')
				return count;
		}

		/* skip whitespace */
		do {
			rc = doFscanc(buf);
			if(rc == IO_EOF)
				return count;
		}
		while(isspace(rc));
		if(doFscanback(buf,rc) == IO_EOF)
			return count;

		/* read flags */
		shortPtr = false;
		discard = false;
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '*':
					discard = true;
					fmt++;
					break;
				case 'h':
					shortPtr = true;
					fmt++;
					break;
				default:
					readFlags = false;
					break;
			}
		}

		/* read length */
		length = 0;
		while(*fmt >= '0' && *fmt <= '9') {
			length = length * 10 + (*fmt - '0');
			fmt++;
		}
		if(length == 0)
			length = -1;

		/* determine format */
		switch(c = *fmt++) {
			/* integers */
			case 'b':
			case 'o':
			case 'x':
			case 'X':
			case 'u':
			case 'd': {
				bool neg = false;
				u32 val = 0;

				/* determine base */
				switch(c) {
					case 'b':
						base = 2;
						break;
					case 'o':
						base = 8;
						break;
					case 'x':
					case 'X':
						base = 16;
						break;
					default:
						base = 10;
						break;
				}

				/* handle '-' */
				if(c == 'd') {
					rc = doFscanc(buf);
					if(rc == IO_EOF)
						return count;
					if(rc == '-') {
						neg = true;
						length--;
					}
					else {
						if(doFscanback(buf,rc) == IO_EOF)
							return count;
					}
				}

				/* read until an invalid char is found or the max length is reached */
				x = 0;
				while(length != 0) {
					rc = doFscanc(buf);
					/* IO_EOF is ok if the stream ends and we've already got a valid number */
					if(rc == IO_EOF)
						break;
					tc = tolower(rc);
					if(tc >= '0' && tc <= numTable[base - 1]) {
						if(base > 10 && tc >= 'a')
							val = val * base + (10 + tc - 'a');
						else
							val = val * base + (tc - '0');
						if(length > 0)
							length--;
						x++;
					}
					else {
						if(doFscanback(buf,rc) == IO_EOF)
							return count;
						break;
					}
				}

				/* valid number? */
				if(x == 0)
					return count;

				/* store value */
				if(!discard) {
					if(c == 'd') {
						n = va_arg(ap, s32*);
						if(shortPtr)
							*(s16*)n = neg ? -val : val;
						else
							*n = neg ? -val : val;
					}
					else {
						u = va_arg(ap, u32*);
						if(shortPtr)
							*(u16*)u = val;
						else
							*u = val;
					}
					count++;
				}
			}
			break;

			/* string */
			case 's':
				if(!discard)
					s = va_arg(ap, char*);

				while(length != 0) {
					rc = doFscanc(buf);
					if(rc == IO_EOF)
						break;
					if(!isspace(rc)) {
						if(!discard)
							*s++ = rc;
						if(length > 0)
							length--;
					}
					else {
						if(doFscanback(buf,rc) == IO_EOF)
							return count;
						break;
					}
				}

				if(!discard) {
					*s = '\0';
					count++;
				}
				break;

			/* character */
			case 'c':
				if(!discard)
					s = va_arg(ap, char*);

				rc = doFscanc(buf);
				if(rc == IO_EOF)
					return count;

				if(!discard) {
					*s = rc;
					count++;
				}
				break;

			/* error */
			default:
				return count;
		}
	}
}

static sIOBuffer *getBuf(tFile *stream) {
	tFD fd = -1;
	tFile **stdStr;
	if(stream == NULL)
		return NULL;

	/* check if it is an uninitialized std-stream */
	switch((u32)stream) {
		case STDIN_NOTINIT:
			fd = STDIN_FILENO;
			stdStr = &stdin;
			break;
		case STDOUT_NOTINIT:
			fd = STDOUT_FILENO;
			stdStr = &stdout;
			break;
		case STDERR_NOTINIT:
			fd = STDERR_FILENO;
			stdStr = &stderr;
			break;
	}

	/* if so, we have to create it */
	if(fd != -1) {
		sIOBuffer *buf = stdBufs + fd;
		if(fd == STDIN_FILENO) {
			buf->out.fd = -1;
			buf->in.fd = fd;
			buf->in.type = BUF_TYPE_FILE;
			buf->in.pos = 0;
			buf->in.max = IN_BUFFER_SIZE;
			buf->in.str = (char*)malloc(IN_BUFFER_SIZE + 1);
			if(buf->in.str == NULL) {
				free(buf);
				return NULL;
			}
		}
		else {
			buf->in.fd = -1;
			buf->out.fd = fd;
			buf->out.type = BUF_TYPE_FILE;
			buf->out.pos = 0;
			buf->out.max = OUT_BUFFER_SIZE;
			buf->out.str = (char*)malloc(OUT_BUFFER_SIZE + 1);
			if(buf->out.str == NULL) {
				free(buf->in.str);
				free(buf);
				return NULL;
			}
		}
		/* store std-stream */
		*stdStr = buf;
		stream = (tFile*)buf;
	}

	return (sIOBuffer*)stream;
}

static sIOBuffer *createBuffer(tFD fd,u8 flags) {
	sIOBuffer *buf;

	/* create list if not already done */
	if(bufList == NULL) {
		bufList = sll_create();
		if(bufList == NULL)
			return NULL;
	}

	/* create new buffer */
	buf = (sIOBuffer*)malloc(sizeof(sIOBuffer));
	if(buf == NULL)
		return NULL;

	/* init in-buffer */
	buf->in.fd = -1;
	if(flags & IO_READ) {
		buf->in.fd = fd;
		buf->in.type = BUF_TYPE_FILE;
		buf->in.pos = 0;
		buf->in.max = IN_BUFFER_SIZE;
		buf->in.str = (char*)malloc(IN_BUFFER_SIZE + 1);
		if(buf->in.str == NULL) {
			free(buf);
			return NULL;
		}
	}

	/* init out-buffer */
	buf->out.fd = -1;
	if(flags & IO_WRITE) {
		buf->out.fd = fd;
		buf->out.type = BUF_TYPE_FILE;
		buf->out.pos = 0;
		buf->out.max = OUT_BUFFER_SIZE;
		buf->out.str = (char*)malloc(OUT_BUFFER_SIZE + 1);
		if(buf->out.str == NULL) {
			free(buf->in.str);
			free(buf);
			return NULL;
		}
	}

	sll_append(bufList,buf);
	return buf;
}

static s32 doFlush(sBuffer *buf) {
	s32 res = 0;
	if(buf->type == BUF_TYPE_FILE && buf->pos > 0) {
		debugBytes(buf->str,buf->pos);
		if(write(buf->fd,buf->str,buf->pos * sizeof(char)) < 0)
			res = IO_EOF;
		buf->pos = 0;
		/* a process switch after we've written some chars seems to be good :) */
		if(res >= 64)
			yield();
	}
	return res;
}
