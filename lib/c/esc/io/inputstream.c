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
#include <esc/mem/heap.h>
#include <esc/io/inputstream.h>
#include <esc/exceptions/io.h>
#include <esc/util/string.h>
#include <esc/esccodes.h>
#include <errors.h>
#include <string.h>
#include <ctype.h>

static s32 istream_reads(sIStream *s,char *str,u32 size);
static s32 istream_readline(sIStream *s,char *str,u32 size);
static s32 istream_readf(sIStream *s,const char *fmt,...);
static s32 istream_vreadf(sIStream *s,const char *fmt,va_list ap);
static s32 istream_readAll(sIStream *s,sString *str);
static s32 istream_readEsc(sIStream *s,s32 *n1,s32 *n2,s32 *n3);
static s32 istream_readfstr(sIStream *s,s32 length,char *str);
static s32 istream_readfnum(sIStream *s,s32 length,char c,s32 *res);

sIStream *istream_open(void) {
	sIStream *s = (sIStream*)heap_alloc(sizeof(sIStream));
	s->reads = istream_reads;
	s->readAll = istream_readAll;
	s->readEsc = istream_readEsc;
	s->readline = istream_readline;
	s->readf = istream_readf;
	s->vreadf = istream_vreadf;
	return s;
}

void istream_close(sIStream *s) {
	heap_free(s);
}

static s32 istream_reads(sIStream *s,char *str,u32 size) {
	char *start = str;
	/* wait for one char left (\0) or till EOF */
	TRY {
		while(size-- > 1)
			*str++ = s->readc(s);
	}
	CATCH(IOException,e) {
		if(e->error != ERR_EOF)
			RETHROW(e);
		/* do nothing */
	}
	ENDCATCH
	*str = '\0';
	return (str - start);
}

static s32 istream_readline(sIStream *s,char *str,u32 size) {
	char *start = str;
	/* wait for one char left (\0) or a newline or EOF */
	TRY {
		char c = 0;
		while(size-- > 1 && (c = s->readc(s)) != '\n')
			*str++ = c;
	}
	CATCH(IOException,e) {
		if(e->error != ERR_EOF)
			RETHROW(e);
		/* do nothing */
	}
	ENDCATCH
	*str = '\0';
	return (str - start);
}

static s32 istream_readf(sIStream *s,const char *fmt,...) {
	va_list ap;
	s32 res;
	va_start(ap,fmt);
	res = istream_vreadf(s,fmt,ap);
	va_end(ap);
	return res;
}

static s32 istream_vreadf(sIStream *s,const char *fmt,va_list ap) {
	char *str = NULL,c,rc = 0;
	s32 *n,count = 0;
	u32 *u,x;
	s32 length;
	bool readFlags;
	bool shortPtr;
	bool discard;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0')
				return count;
			if(c != (rc = s->readc(s)))
				return count;
		}

		/* skip whitespace */
		do {
			rc = s->readc(s);
		}
		while(isspace(rc));
		s->unread(s,rc);

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
				s32 val;
				x = istream_readfnum(s,length,c,&val);
				/* valid number? */
				if(x == 0)
					return count;

				/* store value */
				if(!discard) {
					if(c == 'd') {
						n = va_arg(ap, s32*);
						if(shortPtr)
							*(s16*)n = val;
						else
							*n = val;
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
				str = NULL;
				if(!discard)
					str = va_arg(ap, char*);
				count += istream_readfstr(s,length,str);
				break;

			/* character */
			case 'c':
				if(!discard)
					str = va_arg(ap, char*);

				rc = s->readc(s);
				if(!discard) {
					*str = rc;
					count++;
				}
				break;

			/* error */
			default:
				return count;
		}
	}
}

static s32 istream_readAll(sIStream *s,sString *str) {
	const u32 stepSize = 128;
	u32 size = 0;
	s32 res;
	do {
		size += stepSize;
		/* since we're reading one times more (and get 0) as we have to and call str_resize()
		 * its garanteed that the string is null-terminated */
		str_resize(str,size);
		/* NOTE that we're assume a bit about the internal layout of the string */
		res = s->read(s,str->str + str->len,stepSize - 1);
		str->len += res;
	}
	while(res > 0);
	return str->len;
}

static s32 istream_readEsc(sIStream *s,s32 *n1,s32 *n2,s32 *n3) {
	u32 i;
	char ec,escape[MAX_ESCC_LENGTH] = {0};
	const char *escPtr = (const char*)escape;
	for(i = 0; i < MAX_ESCC_LENGTH - 1 && (ec = s->readc(s)) != ']'; i++)
		escape[i] = ec;
	if(i < MAX_ESCC_LENGTH - 1 && ec == ']')
		escape[i] = ec;
	return escc_get(&escPtr,n1,n2,n3);
}

static s32 istream_readfstr(sIStream *s,s32 length,char *volatile str) {
	TRY {
		/* make copy to prevent gcc to warn about possible clobbering */
		s32 clength = length;
		char rc;
		while(clength != 0) {
			rc = s->readc(s);
			if(!isspace(rc)) {
				if(str)
					*str++ = rc;
				if(clength > 0)
					clength--;
			}
			else {
				s->unread(s,rc);
				break;
			}
		}
	}
	CATCH(IOException,e) {
		/* IO_EOF is ok if the stream ends and we've already got a valid number */
		if(e->error != ERR_EOF)
			RETHROW(e);
	}
	ENDCATCH

	if(str) {
		*str = '\0';
		return 1;
	}
	return 0;
}

static s32 istream_readfnum(sIStream *s,s32 length,char c,s32 *res) {
	const char *numTable = "0123456789abcdef";
	volatile bool neg = false;
	s32 val = 0,x = 0;

	TRY {
		u8 base;
		/* make copy to prevent gcc to warn about possible clobbering */
		s32 clength = length;
		/* handle '-' */
		if(c == 'd') {
			char rc = s->readc(s);
			if(rc == '-') {
				neg = true;
				clength--;
			}
			else
				s->unread(s,rc);
		}

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

		/* read until an invalid char is found or the max length is reached */
		while(clength != 0) {
			char tc = tolower(s->readc(s));
			if(tc >= '0' && tc <= numTable[base - 1]) {
				if(base > 10 && tc >= 'a')
					val = val * base + (10 + tc - 'a');
				else
					val = val * base + (tc - '0');
				if(clength > 0)
					clength--;
				x++;
			}
			else {
				s->unread(s,tc);
				break;
			}
		}
	}
	CATCH(IOException,e) {
		/* IO_EOF is ok if the stream ends and we've already got a valid number */
		if(e->error != ERR_EOF)
			RETHROW(e);
	}
	ENDCATCH
	if(neg)
		val = -val;
	*res = val;
	return x;
}
