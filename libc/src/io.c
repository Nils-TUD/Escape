/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/messages.h"
#include "../h/io.h"
#include "../h/proc.h"
#include "../h/keycodes.h"
#include <string.h>
#include <stdarg.h>

#define BUFFER_SIZE 256

/**
 * Determines the width of the given string
 *
 * @param str the string
 * @return the width
 */
static u8 getswidth(cstring str);

/**
 * Determines the width of the given unsigned 32-bit integer in the given base
 *
 * @param n the integer
 * @param base the base (2..16)
 * @return the width
 */
static u8 getuwidth(u32 n,u8 base);

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
static s8 buffer[BUFFER_SIZE];

static s8 hexCharsBig[] = "0123456789ABCDEF";
static s8 hexCharsSmall[] = "0123456789abcdef";

s32 requestIOPort(u16 port) {
	return requestIOPorts(port,1);
}

s32 releaseIOPort(u16 port) {
	return releaseIOPorts(port,1);
}

s8 readChar(void) {
	s8 c;
	if(read(STDIN_FILENO,&c,sizeof(s8)) <= 0)
		return 0;
	return c;
}

u16 readLine(s8 *line,u16 max) {
	s8 c;
	u8 keycode;
	u8 modifier;
	u16 cursorPos = 0;
	u16 i = 0;
	while(i < max) {
		c = readChar();

		if(handleDefaultEscapeCodes(line,&cursorPos,&i,c,&keycode,&modifier))
			continue;
		/* the above function does not handle all escape-codes */
		if(c == '\033')
			continue;

		/* echo */
		putchar(c);
		flush();

		if(c == '\n')
			break;

		/* not at the end */
		if(cursorPos < i) {
			u32 x;
			/* at first move all one char forward */
			memmove(line + cursorPos + 1,line + cursorPos,i - cursorPos);
			line[cursorPos] = c;
			/* now write the chars to vterm */
			for(x = cursorPos + 1; x <= i; x++)
				putchar(line[x]);
			/* and walk backwards */
			putchar('\033');
			putchar(VK_HOME);
			putchar(i - cursorPos);
			/* we want to do that immediatly */
			flush();
			/* we've added a char */
			cursorPos++;
			i++;
		}
		/* we are at the end of the input */
		else {
			/* put in buffer */
			line[cursorPos++] = c;
			i++;
		}
	}

	line[i] = '\0';
	return i;
}

bool handleDefaultEscapeCodes(s8 *line,u16 *cursorPos,u16 *charcount,s8 c,u8 *keycode,u8 *modifier) {
	bool res = false;
	u16 icursorPos = *cursorPos;
	u16 icharcount = *charcount;
	switch(c) {
		case '\b':
			if(icursorPos > 0) {
				/* remove last char */
				if(icursorPos < icharcount)
					memmove(line + icursorPos - 1,line + icursorPos,icharcount - icursorPos);
				icharcount--;
				icursorPos--;
				line[icharcount] = '\0';
				/* send backspace */
				putchar(c);
				flush();
			}
			res = true;
			break;

		case '\033': {
			bool writeBack = false;
			*keycode = readChar();
			*modifier = readChar();
			switch(*keycode) {
				/* write escape-code back */
				case VK_RIGHT:
					if(icursorPos < icharcount) {
						writeBack = true;
						icursorPos++;
					}
					res = true;
					break;
				case VK_HOME:
					if(icursorPos > 0) {
						writeBack = true;
						/* send the number of chars to move */
						*modifier = icursorPos;
						icursorPos = 0;
					}
					res = true;
					break;
				case VK_END:
					if(icursorPos < icharcount) {
						writeBack = true;
						/* send the number of chars to move */
						*modifier = icharcount - icursorPos;
						icursorPos = icharcount;
					}
					res = true;
					break;
				case VK_LEFT:
					if(icursorPos > 0) {
						writeBack = true;
						icursorPos--;
					}
					res = true;
					break;
			}

			/* write escape-code back */
			if(writeBack) {
				putchar(c);
				putchar(*keycode);
				putchar(*modifier);
				flush();
			}
		}
		break;
	}

	*cursorPos = icursorPos;
	*charcount = icharcount;

	return res;
}

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
			/* finished? */
			if (c == '\0') {
				flush();
				return;
			}
			putchar(c);
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
	buffer[bufferPos++] = c;
}

void flush(void) {
	if(bufferPos > 0) {
		write(STDOUT_FILENO,buffer,bufferPos * sizeof(s8));
		bufferPos = 0;
		/* a process switch improves the performance by far :) */
		yield();
	}
}

void printu(u32 n,u8 base) {
	if(n >= base) {
		printu(n / base,base);
	}
	putchar(hexCharsBig[(n % base)]);
}

void puts(cstring str) {
	s8 c;
	while((c = *str)) {
		putchar(c);
		str++;
	}
}

void printn(s32 n) {
	if(n < 0) {
		putchar('-');
		n = -n;
	}

	if(n >= 10) {
		printn(n / 10);
	}
	putchar('0' + n % 10);
}

static u8 getswidth(cstring str) {
	u8 width = 0;
	while(*str++) {
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
