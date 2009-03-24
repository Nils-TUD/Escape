/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/messages.h"
#include "../h/io.h"
#include "../h/bufio.h"
#include "../h/proc.h"
#include "../h/keycodes.h"
#include "../h/heap.h"
#include <string.h>
#include <stdarg.h>
#include <sllist.h>

/*
 * putchar		printc - fprintc
 * puts			prints - fprints
 * printn		printn - fprintn
 * printu		printu - fprintu
 * printf		printf - fprintf - sprintf
 * vprintf		vprintf - vfprintf - vsprintf
 *
 * sprintf
 *
 * readChar		scanc - fscanc
 * readLine		scanl - fscanl
 * scanf		scanf - fscanf - sscanf
 * 				vscanf - vfscanf - vsscanf
 *
 * flush
 */

/* the number of entries in the hash-map */
#define BUF_MAP_SIZE	8
/* the size of the buffers */
#define BUFFER_SIZE		256

/* an io-buffer for a file-descriptor */
typedef struct {
	tFD fd;
	u16 pos;
	char buffer[BUFFER_SIZE];
} sIOBuffer;

/**
 * Reads one char from <buf>
 *
 * @param buf the buffer
 * @return the character or 0
 */
static char doFscanc(sIOBuffer *buf);

/**
 * Reads one line from <buf> into <line>.
 *
 * @param buf the buffer
 * @param line the line-buffer
 * @param max the max chars to read
 * @return the number of read chars
 */
static u32 doFscans(sIOBuffer *buf,char *line,u32 max);

/**
 * Prints the given character to the buffer. If necessary the buffer is flushed
 *
 * @param buf the io-buffer
 * @param c the character
 * @return the character or the error-code if failed
 */
static s32 doFprintc(sIOBuffer *buf,char c);

/**
 * Prints the given string to the buffer
 *
 * @param buf the io-buffer
 * @param str the string
 * @return the number of printed chars
 */
static s32 doFprints(sIOBuffer *buf,const char *str);

/**
 * Determines the width of the given string
 *
 * @param str the string
 * @return the width
 */
static u8 getswidth(const char *str);

/**
 * Prints the given signed integer to the buffer
 *
 * @param buf the io-buffer
 * @param n the number
 * @return the number of printed chars
 */
static s32 doFprintn(sIOBuffer *buf,s32 n);

/**
 * Determines the width of the given signed 32-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
static u8 getnwidth(s32 n);

/**
 * Prints the given unsigned integer to the given base to the buffer
 *
 * @param buf the io-buffer
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of printed chars
 */
static s32 doFprintu(sIOBuffer *buf,s32 u,u8 base);

/**
 * Same as doFprintu(), but with small letters
 *
 * @param buf the io-buffer
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of printed chars
 */
static s32 doFprintuSmall(sIOBuffer *buf,u32 u,u8 base);

/**
 * Determines the width of the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
static u8 getuwidth(u32 n,u8 base);

/**
 * printf() for the given buffer
 *
 * @param buf the io-buffer
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of printed chars
 */
static s32 doVfprintf(sIOBuffer *buf,const char *fmt,va_list ap);

/**
 * Returns the io-buffer for the given file-desc. If it does not exist it will be created.
 *
 * @param fd the file-descriptor
 * @return the buffer or NULL if failed
 */
static sIOBuffer *getBuffer(tFD fd);

/**
 * Flushes the given buffer
 *
 * @param buf the buffer
 */
static void doFlush(sIOBuffer *buf);

/* for printu() */
static char hexCharsBig[] = "0123456789ABCDEF";
static char hexCharsSmall[] = "0123456789abcdef";

/* buffer for the different file-descriptors */
static sSLList *buffers[BUF_MAP_SIZE] = {NULL};

s32 printf(const char *fmt,...) {
	s32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vfprintf(STDOUT_FILENO,fmt,ap);
	va_end(ap);
	return count;
}

s32 fprintf(tFD fd,const char *fmt,...) {
	s32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vfprintf(fd,fmt,ap);
	va_end(ap);
	return count;
}

s32 vprintf(const char *fmt,va_list ap) {
	return vfprintf(STDOUT_FILENO,fmt,ap);
}

s32 vfprintf(tFD fd,const char *fmt,va_list ap) {
	sIOBuffer *buf;
	buf = getBuffer(fd);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return doVfprintf(buf,fmt,ap);
}

s32 printc(char c) {
	return fprintc(STDOUT_FILENO,c);
}

s32 fprintc(tFD fd,char c) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return doFprintc(buf,c);
}

s32 prints(const char *str) {
	return fprints(STDOUT_FILENO,str);
}

s32 fprints(tFD fd,const char *str) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return doFprints(buf,str);
}

s32 printn(s32 n) {
	return fprintn(STDOUT_FILENO,n);
}

s32 fprintn(tFD fd,s32 n) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return doFprintn(buf,n);
}

s32 printu(s32 u,u8 base) {
	return fprintu(STDOUT_FILENO,u,base);
}

s32 fprintu(tFD fd,s32 u,u8 base) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return doFprintu(buf,u,base);
}

void flush(void) {
	fflush(STDOUT_FILENO);
}

void fflush(tFD fd) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return;
	doFlush(buf);
}

char scanc(void) {
	return fscanc(STDIN_FILENO);
}

char fscanc(tFD fd) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return 0;
	return doFscanc(buf);
}

u32 scans(char *line,u32 max) {
	return fscans(STDIN_FILENO,line,max);
}

u32 fscans(tFD fd,char *line,u32 max) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return 0;
	return doFscans(buf,line,max);
}

static char doFscanc(sIOBuffer *buf) {
	char c;
	if(read(buf->fd,&c,sizeof(char)) <= 0)
		return 0;
	return c;
}

static u32 doFscans(sIOBuffer *buf,char *line,u32 max) {
	char c;
	char *start = line;
	/* wait for one char left (\0) or a newline */
	while(max-- > 1 && (c = doFscanc(buf)) && c != '\n')
		*line++ = c;
	/* terminate */
	*line = '\0';
	return line - start;
}

static s32 doFprintc(sIOBuffer *buf,char c) {
	if(buf->pos >= BUFFER_SIZE)
		doFlush(buf);
	buf->buffer[buf->pos++] = c;
	return c;
}

static s32 doFprints(sIOBuffer *buf,const char *str) {
	char c;
	char *start = (char*)str;
	while((c = *str)) {
		/* handle escape */
		if(c == '\033') {
			doFprintc(buf,c);
			doFprintc(buf,*++str);
			doFprintc(buf,*++str);
		}
		else
			doFprintc(buf,c);
		str++;
	}
	return str - start;
}

static u8 getswidth(const char *str) {
	u8 width = 0;
	while(*str++) {
		width++;
	}
	return width;
}

static s32 doFprintn(sIOBuffer *buf,s32 n) {
	s32 c = 0;
	if(n < 0) {
		doFprintc(buf,'-');
		n = -n;
		c++;
	}

	if(n >= 10) {
		c += doFprintn(buf,n / 10);
	}
	doFprintc(buf,'0' + n % 10);
	return c + 1;
}

static u8 getnwidth(s32 n) {
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

static u8 getuwidth(u32 n,u8 base) {
	u8 width = 1;
	while(n >= base) {
		n /= base;
		width++;
	}
	return width;
}

static s32 doFprintuSmall(sIOBuffer *buf,u32 u,u8 base) {
	s32 c = 0;
	if(u >= base)
		c += doFprintu(buf,u / base,base);
	doFprintc(buf,hexCharsSmall[(u % base)]);
	return c + 1;
}

static s32 doFprintu(sIOBuffer *buf,s32 u,u8 base) {
	s32 c = 0;
	if(u >= base)
		c += doFprintu(buf,u / base,base);
	doFprintc(buf,hexCharsBig[(u % base)]);
	return c + 1;
}

static s32 doVfprintf(sIOBuffer *buf,const char *fmt,va_list ap) {
	char c,b,pad,padchar;
	char *s;
	s32 n;
	u32 u;
	u8 width,base;
	s32 count = 0;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* handle escape */
			if(c == '\033') {
				doFprintc(buf,c);
				doFprintc(buf,*fmt++);
				doFprintc(buf,*fmt++);
				count += 3;
				continue;
			}

			/* finished? */
			if(c == '\0') {
				doFlush(buf);
				return count;
			}
			doFprintc(buf,c);
			count++;
		}

		/* read pad-character */
		pad = 0;
		padchar = ' ';
		if(*fmt == '0' || *fmt == ' ') {
			padchar = *fmt;
			fmt++;
		}

		/* read pad-width */
		while(*fmt >= '0' && *fmt <= '9') {
			pad = pad * 10 + (*fmt - '0');
			fmt++;
		}

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
				n = va_arg(ap, s32);
				if(pad > 0) {
					width = getnwidth(n);
					while(width++ < pad) {
						doFprintc(buf,padchar);
						count++;
					}
				}
				count += doFprintn(buf,n);
				break;
			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				u = va_arg(ap, u32);
				base = c == 'o' ? 8 : (c == 'x' ? 16 : (c == 'b' ? 2 : 10));
				if(pad > 0) {
					width = getuwidth(u,base);
					while(width++ < pad) {
						doFprintc(buf,padchar);
						count++;
					}
				}
				if(c == 'x')
					count += doFprintuSmall(buf,u,base);
				else
					count += doFprintu(buf,u,base);
				break;
			/* string */
			case 's':
				s = va_arg(ap, char*);
				if(pad > 0) {
					width = getswidth(s);
					while(width++ < pad) {
						doFprintc(buf,padchar);
						count++;
					}
				}
				count += doFprints(buf,s);
				break;
			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				doFprintc(buf,b);
				count++;
				break;
			/* all other */
			default:
				doFprintc(buf,c);
				count++;
				break;
		}
	}
}

static sIOBuffer *getBuffer(tFD fd) {
	sSLNode *n;
	sIOBuffer *buf;
	sSLList *list;

	/* search if the buffer exists */
	list = buffers[fd % BUF_MAP_SIZE];
	if(list != NULL) {
		for(n = sll_begin(list); n != NULL; n = n->next) {
			buf = (sIOBuffer*)n->data;
			if(buf->fd == fd)
				return buf;
		}
	}
	else
		list = buffers[fd % BUF_MAP_SIZE] = sll_create();

	if(list == NULL)
		return NULL;

	/* create new buffer */
	buf = (sIOBuffer*)malloc(sizeof(sIOBuffer));
	if(buf == NULL)
		return NULL;

	buf->fd = fd;
	buf->pos = 0;
	sll_append(list,buf);
	return buf;
}

static void doFlush(sIOBuffer *buf) {
	if(buf->pos > 0) {
		write(buf->fd,buf->buffer,buf->pos * sizeof(char));
		buf->pos = 0;
		/* a process switch improves the performance by far :) */
		yield();
	}
}
