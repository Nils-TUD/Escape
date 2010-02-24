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

#include <common.h>
#include <video.h>
#include <machine/serial.h>
#include <util.h>
#include <string.h>
#include <stdarg.h>
#include <esccodes.h>
#include <width.h>

#define VIDEO_BASE			0xC00B8000
#define TAB_WIDTH			4

/* format flags */
#define FFL_PADRIGHT		1
#define FFL_FORCESIGN		2
#define FFL_SPACESIGN		4
#define FFL_PRINTBASE		8
#define FFL_PADZEROS		16
#define FFL_CAPHEX			32

static void vid_printnpad(s32 n,u8 pad,u16 flags);
static void vid_printupad(u32 u,u8 base,u8 pad,u16 flags);
static s32 vid_printpad(s32 count,u16 flags);
static s32 vid_printu(u32 n,u8 base,char *chars);
static s32 vid_printn(s32 n);
static s32 vid_puts(const char *str);
static void vid_clearScreen(void);
static void vid_move(void);
static void vid_handleColorCode(const char **str);
static void vid_removeBIOSCursor(void);

static u16 col = 0;
static u16 row = 0;
static char hexCharsBig[] = "0123456789ABCDEF";
static char hexCharsSmall[] = "0123456789abcdef";
static u8 color = 0;

void vid_init(void) {
	vid_removeBIOSCursor();
	vid_clearScreen();
	color = (BLACK << 4) | WHITE;
}

void vid_putchar(char c) {
	u32 i;
	char *video;
	vid_move();
	video = (char*)(VIDEO_BASE + row * VID_COLS * 2 + col * 2);

#ifdef LOGSERIAL
	/* write to COM1 (some chars make no sense here) */
	if(c != '\r' && c != '\t' && c != '\b')
		ser_out(SER_COM1,c);
#endif

	if(c == '\n') {
		row++;
		vid_putchar('\r');
	}
	else if(c == '\r')
		col = 0;
	else if(c == '\t') {
		i = TAB_WIDTH - col % TAB_WIDTH;
		while(i-- > 0)
			vid_putchar(' ');
	}
	else {
		*video = c;
		video++;
		*video = color;

		/* do an explicit newline if necessary */
		col++;
		if(col >= VID_COLS)
			vid_putchar('\n');
	}
}

void vid_printf(const char *fmt,...) {
	va_list ap;
	va_start(ap, fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
}

void vid_vprintf(const char *fmt,va_list ap) {
	char c,b;
	char *s;
	u8 pad;
	s32 n;
	u32 u;
	u8 width,base;
	bool readFlags;
	u16 flags;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* color-code? */
			if(c == '\033') {
				vid_handleColorCode(&fmt);
				continue;
			}

			/* finished? */
			if(c == '\0')
				return;
			vid_putchar(c);
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
				case '|':
					pad = VID_COLS - col;
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

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
			case 'i':
				n = va_arg(ap, s32);
				vid_printnpad(n,pad,flags);
				break;

			/* pointer */
			case 'p':
				u = va_arg(ap, u32);
				flags |= FFL_PADZEROS;
				pad = 9;
				vid_printupad(u >> 16,16,pad - 5,flags);
				vid_putchar(':');
				vid_printupad(u & 0xFFFF,16,4,flags);
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
				u = va_arg(ap, u32);
				vid_printupad(u,base,pad,flags);
				break;

			/* string */
			case 's':
				s = va_arg(ap, char*);
				if(pad > 0 && !(flags & FFL_PADRIGHT)) {
					width = strlen(s);
					vid_printpad(pad - width,flags);
				}
				n = vid_puts(s);
				if(pad > 0 && (flags & FFL_PADRIGHT))
					vid_printpad(pad - n,flags);
				break;

			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				vid_putchar(b);
				break;

			default:
				vid_putchar(c);
				break;
		}
	}
}

static void vid_printnpad(s32 n,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left */
	if(!(flags & FFL_PADRIGHT) && pad > 0) {
		u32 width = getnwidth(n);
		if(n > 0 && (flags & (FFL_FORCESIGN | FFL_SPACESIGN)))
			width++;
		count += vid_printpad(pad - width,flags);
	}
	/* print '+' or ' ' instead of '-' */
	if(n > 0) {
		if((flags & FFL_FORCESIGN)) {
			vid_putchar('+');
			count++;
		}
		else if(((flags) & FFL_SPACESIGN)) {
			vid_putchar(' ');
			count++;
		}
	}
	/* print number */
	count += vid_printn(n);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		vid_printpad(pad - count,flags);
}

static void vid_printupad(u32 u,u8 base,u8 pad,u16 flags) {
	s32 count = 0;
	/* pad left - spaces */
	if(!(flags & FFL_PADRIGHT) && !(flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getuwidth(u,base);
		count += vid_printpad(pad - width,flags);
	}
	/* print base-prefix */
	if((flags & FFL_PRINTBASE)) {
		if(base == 16 || base == 8) {
			vid_putchar('0');
			count++;
		}
		if(base == 16) {
			char c = (flags & FFL_CAPHEX) ? 'X' : 'x';
			vid_putchar(c);
			count++;
		}
	}
	/* pad left - zeros */
	if(!(flags & FFL_PADRIGHT) && (flags & FFL_PADZEROS) && pad > 0) {
		u32 width = getuwidth(u,base);
		count += vid_printpad(pad - width,flags);
	}
	/* print number */
	if(flags & FFL_CAPHEX)
		count += vid_printu(u,base,hexCharsBig);
	else
		count += vid_printu(u,base,hexCharsSmall);
	/* pad right */
	if((flags & FFL_PADRIGHT) && pad > 0)
		vid_printpad(pad - count,flags);
}

static s32 vid_printpad(s32 count,u16 flags) {
	s32 res = count;
	char c = flags & FFL_PADZEROS ? '0' : ' ';
	while(count-- > 0)
		vid_putchar(c);
	return res;
}

static s32 vid_printu(u32 n,u8 base,char *chars) {
	s32 res = 0;
	if(n >= base)
		res += vid_printu(n / base,base,chars);
	vid_putchar(chars[(n % base)]);
	return res + 1;
}

static s32 vid_printn(s32 n) {
	s32 res = 0;
	if(n < 0) {
		vid_putchar('-');
		n = -n;
		res++;
	}

	if(n >= 10)
		res += vid_printn(n / 10);
	vid_putchar('0' + n % 10);
	return res + 1;
}

static s32 vid_puts(const char *str) {
	const char *begin = str;
	char c;
	while((c = *str)) {
		vid_putchar(c);
		str++;
	}
	return str - begin;
}

static void vid_clearScreen(void) {
	memclear((void*)VIDEO_BASE,VID_COLS * 2 * VID_ROWS);
}

/**
 * Moves all lines one line up, if necessary
 */
static void vid_move(void) {
	/* last line? */
	if(row >= VID_ROWS) {
		/* copy all chars one line back */
		memmove((void*)VIDEO_BASE,(u8*)VIDEO_BASE + VID_COLS * 2,VID_ROWS * VID_COLS * 2);
		row--;
	}
}

static void vid_handleColorCode(const char **str) {
	s32 n1,n2,n3;
	s32 cmd = escc_get(str,&n1,&n2,&n3);
	switch(cmd) {
		case ESCC_COLOR: {
			u8 fg = n1 == ESCC_ARG_UNUSED ? WHITE : MIN(9,n1);
			u8 bg = n2 == ESCC_ARG_UNUSED ? BLACK : MIN(9,n2);
			color = (bg << 4) | fg;
		}
		break;
		default:
			util_panic("Invalid escape-code ^[%d;%d,%d,%d]",cmd,n1,n2,n3);
			break;
	}
}

static void vid_removeBIOSCursor(void) {
	util_outByte(0x3D4,14);
	util_outByte(0x3D5,0x07);
	util_outByte(0x3D4,15);
	util_outByte(0x3D5,0xd0);
}
