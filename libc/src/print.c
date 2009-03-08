/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/messages.h"
#include "../h/io.h"
#include <string.h>
#include <stdarg.h>

#define BUFFER_SIZE 256

typedef struct {
	sMsgDefHeader header;
	s8 chars[BUFFER_SIZE];
} __attribute__((packed)) sSendMsg;

/**
 * Opens the vterm-service
 */
static void init(void);

/**
 * Flushes the buffer
 */
static void flush(void);

/**
 * Writes an escape-code to the vterminal
 *
 * @param str the escape-code
 * @param length the number of chars
 */
static void putEscape(s8 *str,u8 length);

/**
 * Prints the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 */
static void printu(u32 n,u8 base);

/**
 * Determines the width of the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
static u8 getuwidth(u32 n,u8 base);

/**
 * Prints the given string on the screen
 *
 * @param str the string
 */
static void puts(cstring str);

/**
 * Determines the width of the given string
 *
 * @param str the string
 * @return the width
 */
static u8 getswidth(cstring str);

/**
 * Prints the given signed 32-bit integer in base 10
 *
 * @param n the integer
 */
static void printn(s32 n);

/**
 * Determines the width of the given signed 32-bit integer in base 10
 *
 * @param n the integer
 * @return the width
 */
static u8 getnwidth(s32 n);

/**
 * Sames as printu() but with lowercase letters
 *
 * @param n the number
 * @param base the base
 */
static void printuSmall(u32 n,u8 base);

static u16 bufferPos = 0;
static s8 hexCharsBig[] = "0123456789ABCDEF";
static s8 hexCharsSmall[] = "0123456789abcdef";
static s32 termFD = -1;
static sSendMsg msg = {
	.header = {
		.id = MSG_VIDEO_SET,
		.length = 0
	},
	.chars = {0}
};

void printf(cstring fmt,...) {
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt,ap);
	va_end(ap);
}

void vprintf(cstring fmt,va_list ap) {
	s8 c,b,pad,padchar;
	string s;
	s32 n;
	u32 u;
	u8 width,base;

	while (1) {
		/* wait for a '%' */
		while ((c = *fmt++) != '%') {
			/* escape-code? */
			if(c == '\033' || c == '\e') {
				if(*fmt == '[') {
					s8 *escape = fmt - 1;
					fmt++;
					while(*fmt && *fmt != 'm')
						fmt++;
					putEscape(escape,fmt - escape + 1);
					if(*fmt)
						fmt++;
					continue;
				}
			}

			/* finished? */
			if (c == '\0') {
				flush();
				return;
			}
			putchar(c);
		}

		/* read pad-character */
		pad = 0;
		if(*fmt == '0') {
			padchar = '0';
			fmt++;
		}
		else {
			padchar = ' ';
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
						putchar(padchar);
					}
				}
				printn(n);
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
						putchar(padchar);
					}
				}
				if(c == 'x')
					printuSmall(u,base);
				else
					printu(u,base);
				break;
			/* string */
			case 's':
				s = va_arg(ap, string);
				if(pad > 0) {
					width = getswidth(s);
					while(width++ < pad) {
						putchar(padchar);
					}
				}
				puts(s);
				break;
			/* character */
			case 'c':
				b = (s8)va_arg(ap, u32);
				putchar(b);
				break;
			/* all other */
			default:
				putchar(c);
				break;
		}
	}
}

void putchar(s8 c) {
	if(bufferPos >= BUFFER_SIZE - 1)
		flush();
	msg.chars[bufferPos++] = c;
}

static void flush(void) {
	if(termFD == -1)
		init();

	msg.header.length = bufferPos + 1;
	msg.chars[bufferPos] = '\0';
	write(termFD,&msg,sizeof(sMsgDefHeader) + (bufferPos + 1) * sizeof(s8));
	bufferPos = 0;
	/* a process switch improves the performance by far :) */
	yield();
}

static void init(void) {
	do {
		termFD = open("services:/vterm",IO_READ | IO_WRITE);
		if(termFD < 0)
			yield();
	}
	while(termFD < 0);
}

static void putEscape(s8 *str,u8 length) {
	if(bufferPos + length >= BUFFER_SIZE - 1)
		flush();

	while(length-- > 0)
		putchar(*str++);
}

static void printu(u32 n,u8 base) {
	if(n >= base) {
		printu(n / base,base);
	}
	putchar(hexCharsBig[(n % base)]);
}

static u8 getuwidth(u32 n,u8 base) {
	u8 width = 1;
	while(n >= base) {
		n /= base;
		width++;
	}
	return width;
}

static void puts(cstring str) {
	s8 c;
	while((c = *str)) {
		/* escape-code? */
		if(c == '\033' || c == '\e') {
			if(*(str + 1) == '[') {
				s8 *escape = str - 1;
				str += 2;
				while(*str && *str != 'm')
					str++;
				putEscape(escape,str - escape + 1);
				if(*str)
					str++;
			}
		}

		putchar(c);
		str++;
	}
}

static u8 getswidth(cstring str) {
	u8 width = 0;
	while(*str++) {
		width++;
	}
	return width;
}

static void printn(s32 n) {
	if(n < 0) {
		putchar('-');
		n = -n;
	}

	if(n >= 10) {
		printn(n / 10);
	}
	putchar('0' + n % 10);
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

static void printuSmall(u32 n,u8 base) {
	if(n >= base) {
		printuSmall(n / base,base);
	}
	putchar(hexCharsSmall[(n % base)]);
}
