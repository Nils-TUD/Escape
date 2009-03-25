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

/* the number of entries in the hash-map */
#define BUF_MAP_SIZE		8
/* the size of the buffers */
#define IN_BUFFER_SIZE		10
#define OUT_BUFFER_SIZE		256

/* an io-buffer for a file-descriptor */
typedef struct {
	tFD fd;
	u16 inPos;
	u16 outPos;
	char in[IN_BUFFER_SIZE];
	char out[OUT_BUFFER_SIZE];
} sIOBuffer;

/* function that prints a char to somewhere */
typedef s32 (*fPrintChar)(void *ptr,char c);

/**
 * Prints the given character into a buffer. If necessary the buffer is flushed
 *
 * @param ptr the io-buffer
 * @param c the character
 * @return the character or the error-code if failed
 */
static s32 doFprintc(void *ptr,char c);

/**
 * Prints the given character into a string
 *
 * @param ptr the string
 * @param c the character
 * @return the character or the error-code if failed
 */
static s32 doSprintc(void *ptr,char c);

/**
 * Prints the given string
 *
 * @param ptr the io-buffer or the string to write to
 * @param printFunc the function that prints a character
 * @param str the string
 * @return the number of printed chars
 */
static s32 doFprints(void *ptr,fPrintChar printFunc,const char *str);

/**
 * Determines the width of the given string
 *
 * @param str the string
 * @return the width
 */
static u8 getswidth(const char *str);

/**
 * Prints the given signed integer
 *
 * @param ptr the io-buffer or the string to write to
 * @param printFunc the function that prints a character
 * @param n the number
 * @return the number of printed chars
 */
static s32 doFprintn(void *ptr,fPrintChar printFunc,s32 n);

/**
 * Determines the width of the given signed 32-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
static u8 getnwidth(s32 n);

/**
 * Prints the given unsigned integer to the given base
 *
 * @param ptr the io-buffer or the string to write to
 * @param printFunc the function that prints a character
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of printed chars
 */
static s32 doFprintu(void *ptr,fPrintChar printFunc,s32 u,u8 base);

/**
 * Same as doFprintu(), but with small letters
 *
 * @param ptr the io-buffer or the string to write to
 * @param printFunc the function that prints a character
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of printed chars
 */
static s32 doFprintuSmall(void *ptr,fPrintChar printFunc,u32 u,u8 base);

/**
 * Determines the width of the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
static u8 getuwidth(u32 n,u8 base);

/**
 * printf-implementation
 *
 * @param ptr the io-buffer or the string to write to
 * @param printFunc the function that prints a character
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of printed chars
 */
static s32 doVfprintf(void *ptr,fPrintChar printFunc,const char *fmt,va_list ap);

/**
 * Reads one char from <buf>
 *
 * @param buf the buffer
 * @return the character or 0
 */
static char doFscanc(sIOBuffer *buf);

/**
 * Puts the given character back to the scan-buffer
 *
 * @param buf the buffer
 * @param c the character
 */
static void doFscanback(sIOBuffer *buf,char c);

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
 * scanf() for the given buffer
 *
 * @param buf the io-buffer
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of filled vars
 */
static s32 doVfscanf(sIOBuffer *buf,const char *fmt,va_list ap);

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

s32 sprintf(char *str,const char *fmt,...) {
	s32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vsprintf(str,fmt,ap);
	va_end(ap);
	return count;
}

s32 vprintf(const char *fmt,va_list ap) {
	return vfprintf(STDOUT_FILENO,fmt,ap);
}

s32 vfprintf(tFD fd,const char *fmt,va_list ap) {
	s32 res;
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	res = doVfprintf(buf,doFprintc,fmt,ap);
	doFlush(buf);
	return res;
}

s32 vsprintf(char *str,const char *fmt,va_list ap) {
	return doVfprintf(&str,doSprintc,fmt,ap);
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
	return doFprints(buf,doFprintc,str);
}

s32 printn(s32 n) {
	return fprintn(STDOUT_FILENO,n);
}

s32 fprintn(tFD fd,s32 n) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return doFprintn(buf,doFprintc,n);
}

s32 printu(s32 u,u8 base) {
	return fprintu(STDOUT_FILENO,u,base);
}

s32 fprintu(tFD fd,s32 u,u8 base) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return doFprintu(buf,doFprintc,u,base);
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

void scanback(char c) {
	fscanback(STDIN_FILENO,c);
}

void fscanback(tFD fd,char c) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return;
	doFscanback(buf,c);
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

u32 scanf(const char *fmt,...) {
	u32 res;
	va_list ap;
	va_start(ap,fmt);
	res = vfscanf(STDIN_FILENO,fmt,ap);
	va_end(ap);
	return res;
}

u32 fscanf(tFD fd,const char *fmt,...) {
	u32 res;
	va_list ap;
	va_start(ap,fmt);
	res = vfscanf(fd,fmt,ap);
	va_end(ap);
	return res;
}

u32 vscanf(const char *fmt,va_list ap) {
	return vfscanf(STDIN_FILENO,fmt,ap);
}

u32 vfscanf(tFD fd,const char *fmt,va_list ap) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return 0;
	return doVfscanf(buf,fmt,ap);
}

static s32 doFprintc(void *ptr,char c) {
	sIOBuffer *buf = (sIOBuffer*)ptr;
	if(buf->outPos >= OUT_BUFFER_SIZE)
		doFlush(buf);
	buf->out[buf->outPos++] = c;
	return c;
}

static s32 doSprintc(void *ptr,char c) {
	char **strPtr = (char**)ptr;
	char *str = (char*)*strPtr;
	*str = c;
	(*strPtr)++;
	return c;
}

static s32 doFprints(void *ptr,fPrintChar printFunc,const char *str) {
	char c;
	char *start = (char*)str;
	while((c = *str)) {
		/* handle escape */
		if(c == '\033') {
			printFunc(ptr,c);
			printFunc(ptr,*++str);
			printFunc(ptr,*++str);
		}
		else
			printFunc(ptr,c);
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

static s32 doFprintn(void *ptr,fPrintChar printFunc,s32 n) {
	s32 c = 0;
	if(n < 0) {
		printFunc(ptr,'-');
		n = -n;
		c++;
	}

	if(n >= 10) {
		c += doFprintn(ptr,printFunc,n / 10);
	}
	printFunc(ptr,'0' + n % 10);
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

static s32 doFprintuSmall(void *ptr,fPrintChar printFunc,u32 u,u8 base) {
	s32 c = 0;
	if(u >= base)
		c += doFprintuSmall(ptr,printFunc,u / base,base);
	printFunc(ptr,hexCharsSmall[(u % base)]);
	return c + 1;
}

static s32 doFprintu(void *ptr,fPrintChar printFunc,s32 u,u8 base) {
	s32 c = 0;
	if(u >= base)
		c += doFprintu(ptr,printFunc,u / base,base);
	printFunc(ptr,hexCharsBig[(u % base)]);
	return c + 1;
}

static s32 doVfprintf(void *ptr,fPrintChar printFunc,const char *fmt,va_list ap) {
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
				printFunc(ptr,c);
				printFunc(ptr,*fmt++);
				printFunc(ptr,*fmt++);
				count += 3;
				continue;
			}

			/* finished? */
			if(c == '\0') {
				/* strings have to be terminated */
				printFunc(ptr,c);
				return count;
			}
			printFunc(ptr,c);
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
						printFunc(ptr,padchar);
						count++;
					}
				}
				count += doFprintn(ptr,printFunc,n);
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
						printFunc(ptr,padchar);
						count++;
					}
				}
				if(c == 'x')
					count += doFprintuSmall(ptr,printFunc,u,base);
				else
					count += doFprintu(ptr,printFunc,u,base);
				break;
			/* string */
			case 's':
				s = va_arg(ap, char*);
				if(pad > 0) {
					width = getswidth(s);
					while(width++ < pad) {
						printFunc(ptr,padchar);
						count++;
					}
				}
				count += doFprints(ptr,printFunc,s);
				break;
			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				printFunc(ptr,b);
				count++;
				break;
			/* all other */
			default:
				printFunc(ptr,c);
				count++;
				break;
		}
	}
}

static char doFscanc(sIOBuffer *buf) {
	if(buf->inPos > 0)
		return buf->in[--(buf->inPos)];

	char c;
	if(read(buf->fd,&c,sizeof(char)) <= 0)
		return 0;
	return c;
}

static void doFscanback(sIOBuffer *buf,char c) {
	/* ignore scanBacks if the buffer is full */
	/* TODO better way? */
	if(buf->inPos >= IN_BUFFER_SIZE)
		return;
	buf->in[(buf->inPos)++] = c;
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

static s32 doVfscanf(sIOBuffer *buf,const char *fmt,va_list ap) {
	char *s,c,rc = 0,length;
	const char *numTable = "0123456789abcdef";
	s32 *n,count = 0;
	u32 *u,x;
	u8 base;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			if(c != doFscanc(buf))
				return count;
			/* finished? */
			if(c == '\0')
				return count;
		}

		/* skip whitespace */
		do {
			rc = doFscanc(buf);
		}
		while(rc == ' ' || rc == '\t');
		doFscanback(buf,rc);

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
					if(rc == '-') {
						neg = true;
						length--;
					}
					else
						doFscanback(buf,rc);
				}

				/* read until an invalid char is found or the max length is reached */
				x = 0;
				while(length != 0) {
					rc = doFscanc(buf);
					if(rc >= numTable[0] && rc <= numTable[base - 1]) {
						val = val * base + (rc - numTable[0]);
						if(length > 0)
							length--;
						x++;
					}
					else {
						doFscanback(buf,rc);
						break;
					}
				}

				/* valid number? */
				if(x == 0)
					return count;

				/* store value */
				if(c == 'd') {
					n = va_arg(ap, s32*);
					*n = neg ? -val : val;
				}
				else {
					u = va_arg(ap, u32*);
					*u = val;
				}
				count++;
			}
			break;

			/* string */
			case 's':
				s = va_arg(ap, char*);
				while(length != 0) {
					rc = doFscanc(buf);
					if(!isspace(rc)) {
						*s++ = rc;
						if(length > 0)
							length--;
					}
					else {
						doFscanback(buf,rc);
						break;
					}
				}
				*s = '\0';
				count++;
				break;

			/* character */
			case 'c':
				s = va_arg(ap, char*);
				*s = doFscanc(buf);
				count++;
				break;

			/* error */
			default:
				return count;
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
	buf->inPos = 0;
	buf->outPos = 0;
	sll_append(list,buf);
	return buf;
}

static void doFlush(sIOBuffer *buf) {
	if(buf->outPos > 0) {
		write(buf->fd,buf->out,buf->outPos * sizeof(char));
		buf->outPos = 0;
		/* a process switch improves the performance by far :) */
		yield();
	}
}
