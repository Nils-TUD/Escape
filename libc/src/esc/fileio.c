/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/proc.h>
#include <esc/keycodes.h>
#include <esc/heap.h>
#include <string.h>
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
#define FFL_PADRIGHT	1
#define FFL_FORCESIGN	2
#define FFL_SPACESIGN	4
#define FFL_PRINTBASE	8
#define FFL_PADZEROS	16
#define FFL_CAPHEX		32
#define FFL_SHORT		64
#define FFL_LONG		128

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
static s32 doFprints(sBuffer *buf,const char *str);

/**
 * Determines the width of the given string
 *
 * @param str the string
 * @return the width
 */
static u8 getswidth(const char *str);

/**
 * Prints the given signed integer with padding
 *
 * @param buf the buffer to write to
 * @param n the number
 * @param pad the total width of the number
 * @param flags the format-flags
 * @return the number of printed chars
 */
static s32 doFprintnPad(sBuffer *buf,s32 n,u8 pad,u16 flags);

/**
 * Prints the given signed integer
 *
 * @param buf the buffer to write to
 * @param n the number
 * @return the number of printed chars
 */
static s32 doFprintn(sBuffer *buf,s32 n);

/**
 * Determines the width of the given signed 32-bit integer in base 10
 *
 * @param n the integer
 * @param flags the format-flags
 * @return the width
 */
static u8 getnwidth(s32 n,u16 flags);

/**
 * Prints the given unsigned integer to the given base with padding
 *
 * @param buf the buffer to write to
 * @param u the number
 * @param base the base (2 .. 16)
 * @param pad the total width of the number
 * @param flags the format-flags
 * @return the number of printed chars
 */
static s32 doFprintuPad(sBuffer *buf,u32 u,u8 base,u8 pad,u16 flags);

/**
 * Prints the given unsigned integer to the given base
 *
 * @param buf the buffer to write to
 * @param u the number
 * @param base the base (2 .. 16)
 * @param hexchars pointer to the string with hex-digits that should be used
 * @return the number of printed chars
 */
static s32 doFprintu(sBuffer *buf,u32 u,u8 base,char *hexchars);

/**
 * Adds padding at the current position
 *
 * @param buf the buffer to write to
 * @param count the number of pad-chars to write
 * @param flags the format-flags
 * @return the number of written chars
 */
static s32 doPad(sBuffer *buf,s32 count,u16 flags);

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
static s32 doVfprintf(sBuffer *buf,const char *fmt,va_list ap);

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
static s32 doFscanl(sBuffer *buf,char *line,u32 max);

/**
 * Reads <max> chars from <buf> into <buffer>.
 *
 * @param buf the buffer
 * @param buffer the buffer
 * @param max the max chars to read
 * @return the number of read chars
 */
static s32 doFscans(sBuffer *buf,char *buffer,u32 max);

/**
 * scanf() for the given buffer
 *
 * @param buf the io-buffer
 * @param fmt the format
 * @param ap the argument-list
 * @return the number of filled vars
 */
static s32 doVfscanf(sBuffer *buf,const char *fmt,va_list ap);

/**
 * If <stream> is an stdin, stdout or stderr and is not initialized, it will be done.
 * Otherwise nothing is done
 *
 * @param stream the stream
 * @return the stream or NULL
 */
static sIOBuffer *getBuf(tFile *stream);

/**
 * Creates an io-buffer for the given file-desc
 *
 * @param fd the file-descriptor
 * @param flags the flags with which the file was opened
 * @return the buffer or NULL if failed
 */
static sIOBuffer *createBuffer(tFD fd,u8 flags);

/**
 * Flushes the given buffer
 *
 * @param buf the buffer
 * @return 0 on success
 */
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
	char *msg;
	va_list ap;

	msg = strerror(errno);
	/* write char for testing */
	fprintc(stderr,' ');
	res = fflush(stderr);
	/* if this failed, maybe we have no stderr (just the shell and childs have it) */
	/* so try debugf */
	if(res != 0) {
		va_start(ap,prefix);
		vdebugf(prefix,ap);
		va_end(ap);
		debugf(": %s\n",msg);
		res = 0;
	}
	else {
		fprintc(stderr,'\r');
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

static u8 getswidth(const char *str) {
	u8 width = 0;
	while(*str++) {
		width++;
	}
	return width;
}

static s32 doFprintnPad(sBuffer *buf,s32 n,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		u32 width = getnwidth(n,flags);
		count += doPad(buf,pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	if(n > 0) {
		if((flags & FFL_FORCESIGN)) {
			if(doFprintc(buf,'+') == IO_EOF)
				return count;
			count++;
		}
		else if((flags & FFL_SPACESIGN)) {
			if(doFprintc(buf,' ') == IO_EOF)
				return count;
			count++;
		}
	}
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

static u8 getnwidth(s32 n,u16 flags) {
	/* we have at least one char */
	u8 width = 1;
	if(n > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
		width++;
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

static s32 doFprintuPad(sBuffer *buf,u32 u,u8 base,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getuwidth(u,base);
		count += doPad(buf,pad - width,flags);
	}
	/* print base-prefix */
	if((flags & FFL_PRINTBASE)) {
		if(base == 16 || base == 8) {
			if(doFprintc(buf,'0') == IO_EOF)
				return count;
			count++;
		}
		if(base == 16) {
			char c = (flags & FFL_CAPHEX) ? 'X' : 'x';
			if(doFprintc(buf,c) == IO_EOF)
				return count;
			count++;
		}
	}
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

static s32 doVfprintf(sBuffer *buf,const char *fmt,va_list ap) {
	char c,b,pad;
	char *s;
	s32 *ptr;
	s32 n;
	u32 u;
	bool readFlags;
	u16 flags;
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

		/* read length */
		switch(*fmt) {
			case 'l':
				flags |= FFL_LONG;
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
				n = va_arg(ap, s32);
				if(flags & FFL_SHORT)
					n &= 0xFFFF;
				count += doFprintnPad(buf,n,pad,flags);
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

			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				u = va_arg(ap, u32);
				if(flags & FFL_SHORT)
					u &= 0xFFFF;
				base = c == 'o' ? 8 : ((c == 'x' || c == 'X') ? 16 : (c == 'b' ? 2 : 10));
				if(c == 'X')
					flags |= FFL_CAPHEX;
				count += doFprintuPad(buf,u,base,pad,flags);
				break;
			/* string */
			case 's':
				s = va_arg(ap, char*);
				if(pad > 0 && !(flags & FFL_PADRIGHT)) {
					width = getswidth(s);
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
	char *s,c,tc,rc = 0,length;
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
		if(write(buf->fd,buf->str,buf->pos * sizeof(char)) < 0)
			res = IO_EOF;
		buf->pos = 0;
		/* a process switch improves the performance by far :) */
		if(res >= 0)
			yield();
	}
	return res;
}
