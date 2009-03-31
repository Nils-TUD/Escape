/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/bufio.h>
#include <esc/proc.h>
#include <esc/keycodes.h>
#include <esc/heap.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <sllist.h>

/* the number of entries in the hash-map */
#define BUF_MAP_SIZE		8
/* the size of the buffers */
#define IN_BUFFER_SIZE		10
#define OUT_BUFFER_SIZE		256

#define BUF_TYPE_STRING		0
#define BUF_TYPE_FILE		1

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

/**
 * Prints the given character into a buffer
 *
 * @param buf the buffer to write to
 * @param c the character
 * @return the character or IO_EOF if failed
 */
static char doFprintc(sBuffer *buf,char c);

/**
 * Prints the given string
 *
 * @param buf the buffer to write to
 * @param str the string
 * @return the number of printed chars
 */
static u32 doFprints(sBuffer *buf,const char *str);

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
 * @param buf the buffer to write to
 * @param n the number
 * @return the number of printed chars
 */
static u32 doFprintn(sBuffer *buf,s32 n);

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
 * @param buf the buffer to write to
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of printed chars
 */
static u32 doFprintu(sBuffer *buf,s32 u,u8 base);

/**
 * Same as doFprintu(), but with small letters
 *
 * @param buf the buffer to write to
 * @param u the number
 * @param base the base (2 .. 16)
 * @return the number of printed chars
 */
static u32 doFprintuSmall(sBuffer *buf,u32 u,u8 base);

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
 * @param buf the buffer to write to
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of printed chars
 */
static u32 doVfprintf(sBuffer *buf,const char *fmt,va_list ap);

/**
 * Reads one char from <buf>
 *
 * @param buf the buffer
 * @return the character or IO_EOF
 */
static char doFscanc(sBuffer *buf);

/**
 * Puts the given character back to the scan-buffer
 *
 * @param buf the buffer
 * @param c the character
 * @param 0 on success, IO_EOF on error
 */
static s32 doFscanback(sBuffer *buf,char c);

/**
 * Reads one line from <buf> into <line>.
 *
 * @param buf the buffer
 * @param line the line-buffer
 * @param max the max chars to read
 * @return the number of read chars
 */
static u32 doFscanl(sBuffer *buf,char *line,u32 max);

/**
 * Reads <max> chars from <buf> into <buffer>.
 *
 * @param buf the buffer
 * @param buffer the buffer
 * @param max the max chars to read
 * @return the number of read chars
 */
static u32 doFscans(sBuffer *buf,char *buffer,u32 max);

/**
 * scanf() for the given buffer
 *
 * @param buf the io-buffer
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of filled vars
 */
static u32 doVfscanf(sBuffer *buf,const char *fmt,va_list ap);

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
static void doFlush(sBuffer *buf);

/* for printu() */
static char hexCharsBig[] = "0123456789ABCDEF";
static char hexCharsSmall[] = "0123456789abcdef";

/* buffer for the different file-descriptors */
static sSLList *buffers[BUF_MAP_SIZE] = {NULL};

u32 printf(const char *fmt,...) {
	u32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vfprintf(STDOUT_FILENO,fmt,ap);
	va_end(ap);
	return count;
}

u32 fprintf(tFD fd,const char *fmt,...) {
	u32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vfprintf(fd,fmt,ap);
	va_end(ap);
	return count;
}

u32 sprintf(char *str,const char *fmt,...) {
	u32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vsnprintf(str,-1,fmt,ap);
	va_end(ap);
	return count;
}

u32 snprintf(char *str,u32 max,const char *fmt,...) {
	u32 count;
	va_list ap;
	va_start(ap,fmt);
	count = vsnprintf(str,max,fmt,ap);
	va_end(ap);
	return count;
}

u32 vprintf(const char *fmt,va_list ap) {
	return vfprintf(STDOUT_FILENO,fmt,ap);
}

u32 vfprintf(tFD fd,const char *fmt,va_list ap) {
	u32 res;
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return IO_EOF;
	res = doVfprintf(&(buf->out),fmt,ap);
	doFlush(&(buf->out));
	return res;
}

u32 vsprintf(char *str,const char *fmt,va_list ap) {
	return vsnprintf(str,-1,fmt,ap);
}

u32 vsnprintf(char *str,u32 max,const char *fmt,va_list ap) {
	sBuffer buf = {
		.type = BUF_TYPE_STRING,
		.str = str,
		.pos = 0,
		.max = max
	};
	u32 res = doVfprintf(&buf,fmt,ap);
	/* terminate */
	buf.str[buf.pos] = '\0';
	return res;
}

char printc(char c) {
	return fprintc(STDOUT_FILENO,c);
}

char fprintc(tFD fd,char c) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return IO_EOF;
	return doFprintc(&(buf->out),c);
}

u32 prints(const char *str) {
	return fprints(STDOUT_FILENO,str);
}

u32 fprints(tFD fd,const char *str) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return IO_EOF;
	return doFprints(&(buf->out),str);
}

u32 printn(s32 n) {
	return fprintn(STDOUT_FILENO,n);
}

u32 fprintn(tFD fd,s32 n) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return IO_EOF;
	return doFprintn(&(buf->out),n);
}

u32 printu(s32 u,u8 base) {
	return fprintu(STDOUT_FILENO,u,base);
}

u32 fprintu(tFD fd,s32 u,u8 base) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return IO_EOF;
	return doFprintu(&(buf->out),u,base);
}

void flush(void) {
	fflush(STDOUT_FILENO);
}

void fflush(tFD fd) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return;
	doFlush(&(buf->out));
}

char scanc(void) {
	return fscanc(STDIN_FILENO);
}

char fscanc(tFD fd) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return IO_EOF;
	return doFscanc(&(buf->in));
}

s32 scanback(char c) {
	return fscanback(STDIN_FILENO,c);
}

s32 fscanback(tFD fd,char c) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return IO_EOF;
	return doFscanback(&(buf->in),c);
}

u32 scanl(char *line,u32 max) {
	return fscanl(STDIN_FILENO,line,max);
}

u32 fscanl(tFD fd,char *line,u32 max) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return 0;
	return doFscanl(&(buf->in),line,max);
}

u32 scans(char *buffer,u32 max) {
	return fscans(STDIN_FILENO,buffer,max);
}

u32 fscans(tFD fd,char *buffer,u32 max) {
	sIOBuffer *buf = getBuffer(fd);
	if(buf == NULL)
		return 0;
	return doFscans(&(buf->in),buffer,max);
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

u32 sscanf(const char *str,const char *fmt,...) {
	u32 res;
	va_list ap;
	va_start(ap,fmt);
	res = vsscanf(str,fmt,ap);
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
	return doVfscanf(&(buf->in),fmt,ap);
}

u32 vsscanf(const char *str,const char *fmt,va_list ap) {
	sBuffer buf = {
		.type = BUF_TYPE_STRING,
		.pos = 0,
		.max = -1,
		.str = (char*)str
	};
	return doVfscanf(&buf,fmt,ap);
}

static char doFprintc(sBuffer *buf,char c) {
	if(buf->max != -1 && buf->pos >= buf->max) {
		if(buf->type == BUF_TYPE_FILE)
			doFlush(buf);
		else
			return IO_EOF;
	}
	buf->str[buf->pos++] = c;
	return c;
}

static u32 doFprints(sBuffer *buf,const char *str) {
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

static u8 getswidth(const char *str) {
	u8 width = 0;
	while(*str++) {
		width++;
	}
	return width;
}

static u32 doFprintn(sBuffer *buf,s32 n) {
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

static u32 doFprintuSmall(sBuffer *buf,u32 u,u8 base) {
	u32 c = 0;
	if(u >= base)
		c += doFprintuSmall(buf,u / base,base);
	if(doFprintc(buf,hexCharsSmall[(u % base)]) == IO_EOF)
		return c;
	return c + 1;
}

static u32 doFprintu(sBuffer *buf,s32 u,u8 base) {
	u32 c = 0;
	if(u >= base)
		c += doFprintu(buf,u / base,base);
	if(doFprintc(buf,hexCharsBig[(u % base)]) == IO_EOF)
		return c;
	return c + 1;
}

static u32 doVfprintf(sBuffer *buf,const char *fmt,va_list ap) {
	char c,b,pad,padchar;
	char *s;
	s32 n;
	u32 u;
	u8 width,base;
	u32 count = 0;

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
						if(doFprintc(buf,padchar) == IO_EOF)
							return count;
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
				base = c == 'o' ? 8 : ((c == 'x' || c == 'X') ? 16 : (c == 'b' ? 2 : 10));
				if(pad > 0) {
					width = getuwidth(u,base);
					while(width++ < pad) {
						if(doFprintc(buf,padchar) == IO_EOF)
							return count;
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
						if(doFprintc(buf,padchar) == IO_EOF)
							return count;
						count++;
					}
				}
				count += doFprints(buf,s);
				break;
			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				if(doFprintc(buf,b) == IO_EOF)
					return count;
				count++;
				break;
			/* all other */
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

static u32 doFscanl(sBuffer *buf,char *line,u32 max) {
	char c;
	char *start = line;
	/* wait for one char left (\0) or a newline or EOF */
	while(max-- > 1 && (c = doFscanc(buf)) != IO_EOF && c != '\n')
		*line++ = c;
	/* terminate */
	*line = '\0';
	return line - start;
}

static u32 doFscans(sBuffer *buf,char *buffer,u32 max) {
	char c;
	char *start = buffer;
	/* wait for one char left (\0) or till EOF */
	while(max-- > 1 && (c = doFscanc(buf)) != IO_EOF)
		*buffer++ = c;
	/* terminate */
	*buffer = '\0';
	return buffer - start;
}

static u32 doVfscanf(sBuffer *buf,const char *fmt,va_list ap) {
	char *s,c,tc,rc = 0,length;
	const char *numTable = "0123456789abcdef";
	s32 *n;
	u32 *u,x,count = 0;
	u8 base;

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
		while(rc == ' ' || rc == '\t');
		if(doFscanback(buf,rc) == IO_EOF)
			return count;

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
					if(rc == IO_EOF)
						break;
					if(!isspace(rc)) {
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
				*s = '\0';
				count++;
				break;

			/* character */
			case 'c':
				s = va_arg(ap, char*);
				rc = doFscanc(buf);
				if(rc == IO_EOF)
					return count;
				*s = rc;
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
			/* we can use in.fd here because in.fd and out.fd is always the same */
			if(buf->in.fd == fd)
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

	/* init in-buffer */
	buf->in.fd = fd;
	buf->in.type = BUF_TYPE_FILE;
	buf->in.pos = 0;
	buf->in.max = IN_BUFFER_SIZE;
	buf->in.str = (char*)malloc(IN_BUFFER_SIZE + 1);
	if(buf->in.str == NULL) {
		free(buf);
		return NULL;
	}

	/* init out-buffer */
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

	sll_append(list,buf);
	return buf;
}

static void doFlush(sBuffer *buf) {
	if(buf->type == BUF_TYPE_FILE && buf->pos > 0) {
		write(buf->fd,buf->str,buf->pos * sizeof(char));
		buf->pos = 0;
		/* a process switch improves the performance by far :) */
		yield();
	}
}
