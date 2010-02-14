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

#define VIDEO_BASE	0xC00B8000
#define COL_WOB		0x07				/* white on black */
#define COLS		80
#define ROWS		25
#define TAB_WIDTH	4

/* special escape-codes */
#define ESC_FG		'f'
#define ESC_BG		'b'
#define ESC_RESET	'r'

/**
 * Handles a color-code
 *
 * @param str a pointer to the current string-position (will be changed)
 */
static void vid_handleColorCode(const char **str);

/**
 * Removes the BIOS-cursor
 */
static void vid_removeBIOSCursor(void);

/**
 * Sames as vid_printu() but with lowercase letters
 *
 * @param n the number
 * @param base the base
 */
static void vid_printuSmall(u32 n,u8 base);

static u16 col = 0;
static u16 row = 0;
static char hexCharsBig[] = "0123456789ABCDEF";
static char hexCharsSmall[] = "0123456789abcdef";
static u8 color = 0;
static u8 oldBG = 0, oldFG = 0;

void vid_init(void) {
	vid_removeBIOSCursor();
	vid_clearScreen();
	vid_setFGColor(WHITE);
	vid_setBGColor(BLACK);
}

void vid_clearScreen(void) {
	memclear((void*)VIDEO_BASE,COLS * 2 * ROWS);
}

void vid_useColor(eColor bg,eColor fg) {
	oldBG = vid_getBGColor();
	oldFG = vid_getFGColor();
	vid_setBGColor(bg);
	vid_setFGColor(fg);
}

void vid_restoreColor(void) {
	vid_setBGColor((eColor)oldBG);
	vid_setFGColor((eColor)oldFG);
}

eColor vid_getFGColor(void) {
	return (eColor)(color & 0xF);
}

eColor vid_getBGColor(void) {
	return (eColor)((color >> 4) & 0xF);
}

void vid_setFGColor(eColor ncol) {
	color = (color & 0xF0) | (ncol & 0xF);
}

void vid_setBGColor(eColor ncol) {
	color = (color & 0x0F) | ((ncol << 4) & 0xF0);
}

void vid_setLineBG(u8 line,eColor bg) {
	u8 ncol = ((bg << 4) & 0xF0) | color;
	u8 *addr = (u8*)(VIDEO_BASE + line * COLS * 2);
	u8 *end = (u8*)((u32)addr + COLS * 2);
	for(addr++; addr < end; addr += 2) {
		*addr = ncol;
	}
}

u8 vid_getLine(void) {
	char *video = (char*)(VIDEO_BASE + row * COLS * 2 + col * 2);
	return ((u32)video - VIDEO_BASE) / (COLS * 2);
}

void vid_toLineEnd(u8 pad) {
	col = COLS - pad;
}

/**
 * Moves all lines one line up, if necessary
 */
static void vid_move(void) {
	/* last line? */
	if(row >= ROWS) {
		/* copy all chars one line back */
		memmove((void*)VIDEO_BASE,(u8*)VIDEO_BASE + COLS * 2,ROWS * COLS * 2);
		row--;
	}
}

void vid_putchar(char c) {
	u32 i;
	char *video;
	vid_move();
	video = (char*)(VIDEO_BASE + row * COLS * 2 + col * 2);

#ifdef LOGSERIAL
	/* write to COM1 (some chars make no sense here) */
	if(c != '\r' && c != '\t' && c != '\b')
		ser_out(SER_COM1,c);
#endif

	if(c == '\n') {
		/* to next line */
		row++;
		/* move cursor to line start */
		vid_putchar('\r');
	}
	else if(c == '\r') {
		/* to line-start */
		col = 0;
	}
	else if(c == '\t') {
		i = TAB_WIDTH - col % TAB_WIDTH;
		while(i-- > 0) {
			vid_putchar(' ');
		}
	}
	else {
		*video = c;
		video++;
		*video = color;

		/* do an explicit newline if necessary */
		col++;
		if(col >= COLS)
			vid_putchar('\n');
	}
}

void vid_printu(u32 n,u8 base) {
	if(n >= base) {
		vid_printu(n / base,base);
	}
	vid_putchar(hexCharsBig[(n % base)]);
}

void vid_puts(const char *str) {
	char c;
	while((c = *str)) {
		/* color-code? */
		if(c == '\033') {
			str++;
			vid_handleColorCode(&str);
			continue;
		}

		vid_putchar(c);
		str++;
	}
}

void vid_printn(s32 n) {
	if(n < 0) {
		vid_putchar('-');
		n = -n;
	}

	if(n >= 10) {
		vid_printn(n / 10);
	}
	vid_putchar('0' + n % 10);
}

void vid_printf(const char *fmt,...) {
	va_list ap;
	va_start(ap, fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
}

void vid_vprintf(const char *fmt,va_list ap) {
	char c,b,padchar;
	char *s;
	u8 pad;
	s32 n;
	u32 u;
	u8 width,base;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* color-code? */
			if(c == '\033') {
				vid_handleColorCode(&fmt);
				continue;
			}

			/* finished? */
			if(c == '\0') {
				return;
			}
			vid_putchar(c);
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
						vid_putchar(padchar);
					}
				}
				vid_printn(n);
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
						vid_putchar(padchar);
					}
				}
				if(c == 'x')
					vid_printuSmall(u,base);
				else
					vid_printu(u,base);
				break;
			/* string */
			case 's':
				s = va_arg(ap, char*);
				if(pad > 0) {
					width = strlen(s);
					while(width++ < pad) {
						vid_putchar(padchar);
					}
				}
				vid_puts(s);
				break;
			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				vid_putchar(b);
				break;
			/* all other */
			default:
				vid_putchar(c);
				break;
		}
	}
}

static void vid_handleColorCode(const char **str) {
	s32 n1,n2,n3;
	s32 cmd = escc_get(str,&n1,&n2,&n3);
	switch(cmd) {
		case ESCC_COLOR:
			vid_setFGColor(n1 == ESCC_ARG_UNUSED ? WHITE : MIN(9,n1));
			vid_setBGColor(n2 == ESCC_ARG_UNUSED ? BLACK : MIN(9,n2));
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

static void vid_printuSmall(u32 n,u8 base) {
	if(n >= base) {
		vid_printuSmall(n / base,base);
	}
	vid_putchar(hexCharsSmall[(n % base)]);
}
