/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/video.h"
#include "../h/common.h"
#include "../h/util.h"
#include <stdarg.h>

#define COL_WOB 0x07				/* white on black */
#define VIDEO_BASE 0xC00B8000
#define COLS 80
#define ROWS 25
#define TAB_WIDTH 2

static s8 *video = (s8*)VIDEO_BASE;
static s8 hexChars[] = "0123456789ABCDEF";
static u8 color = 0;

static u8 oldBG = 0, oldFG = 0;

/**
 * Removes the BIOS-cursor from the screen
 */
static void vid_removeBIOSCursor(void) {
	outb(0x3D4,14);
	outb(0x3D5,0x07);
	outb(0x3D4,15);
	outb(0x3D5,0xd0);
}

void vid_init(void) {
	vid_removeBIOSCursor();
	vid_clearScreen();
	vid_setFGColor(WHITE);
	vid_setBGColor(BLACK);
}

void vid_clearScreen(void) {
	u8 *addr = (u8*)VIDEO_BASE;
	u8 *end = (u8*)((u32)addr + COLS * 2 * ROWS);
	for(; addr < end; addr++) {
		*addr = 0;
	}
}

void vid_useColor(tColor bg,tColor fg) {
	oldBG = vid_getBGColor();
	oldFG = vid_getFGColor();
	vid_setBGColor(bg);
	vid_setFGColor(fg);
}

void vid_restoreColor(void) {
	vid_setBGColor(oldBG);
	vid_setFGColor(oldFG);
}

tColor vid_getFGColor(void) {
	return color & 0xF;
}

tColor vid_getBGColor(void) {
	return (color >> 4) & 0xF;
}

void vid_setFGColor(tColor col) {
	color |= col & 0xF;
}

void vid_setBGColor(tColor col) {
	color |= (col << 4) & 0xF0;
}

void vid_setLineBG(u8 line,tColor bg) {
	u8 col = ((bg << 4) & 0xF0) | color;
	u8 *addr = (u8*)(VIDEO_BASE + line * COLS * 2);
	u8 *end = (u8*)((u32)addr + COLS * 2);
	for(addr++; addr < end; addr += 2) {
		*addr = col;
	}
}

u8 vid_getLine(void) {
	return ((u32)video - VIDEO_BASE) / (COLS * 2);
}

void vid_toLineEnd(u8 pad) {
	u16 col = (u32)(video - VIDEO_BASE) % (COLS * 2);
	video = (s8*)((u32)video + ((COLS * 2) - col) - pad * 2);
}

/**
 * Moves all lines one line up, if necessary
 */
static void vid_move(void) {
	u32 i;
	s8 *src,*dst;
	/* last line? */
	if(video >= (s8*)(VIDEO_BASE + (ROWS - 1) * COLS * 2)) {
		/* copy all chars one line back */
		src = (s8*)(VIDEO_BASE + COLS * 2);
		dst = (s8*)VIDEO_BASE;
		for(i = 0; i < ROWS * COLS * 2; i++) {
			*dst++ = *src++;
		}
		/* to prev line */
		video -= COLS * 2;
	}
}

void vid_putchar(s8 c) {
	u32 i;
	vid_move();
	
	/* write to bochs/qemu console (\r not necessary here) */
	if(c != '\r') {
		outb(0xe9,c);
	    outb(0x3f8,c);
	    while((inb(0x3fd) & 0x20) == 0);
	}
    
	if(c == '\n') {
		/* to next line */
		video += COLS * 2;
		/* move cursor to line start */
		vid_putchar('\r');
	}
	else if(c == '\r') {
		/* to line-start */
		video -= (u32)(video - VIDEO_BASE) % (COLS * 2);
	}
	else if(c == '\t') {
		i = TAB_WIDTH;
		while(i-- > 0) {
			vid_putchar(' ');
		}
	}
	else {
		*video = c;
		video++;
		*video = color;
		video++;
	}
}

void vid_printu(u32 n,u8 base) {
	if(n >= base) {
		vid_printu(n / base,base);
	}
	vid_putchar(hexChars[(n % base)]);
}

u8 vid_getuwidth(u32 n,u8 base) {
	u8 width = 1;
	while(n >= base) {
		n /= base;
		width++;
	}
	return width;
}

void vid_puts(s8 *str) {
	while(*str) {
		vid_putchar(*str++);
	}
}

u8 vid_getswidth(s8 *str) {
	u8 width = 0;
	while(*str++) {
		width++;
	}
	return width;
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

u8 vid_getnwidth(s32 n) {
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

void vid_printf(s8 *fmt,...) {
	va_list ap;
	s8 c,b,*s,oldcolor = color,pad,padchar;
	s32 n;
	u32 u;
	u8 width,base;
	
	va_start(ap, fmt);
	while (1) {
		/* wait for a '%' */
		while ((c = *fmt++) != '%') {
			/* finished? */
			if (c == '\0') {
				va_end(ap);
				return;
			}
			vid_putchar(c);
		}
		
		/* color given? */
		if(*fmt == ':') {
			color = ((*(++fmt) - '0') & 0xF) << 4;
			color |= (*(++fmt) - '0') & 0xF;
			fmt++;
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
					width = vid_getnwidth(n);
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
				u = va_arg(ap, u32);
				base = c == 'o' ? 8 : (c == 'x' ? 16 : (c == 'b' ? 2 : 10));
				if(pad > 0) {
					width = vid_getuwidth(u,base);
					while(width++ < pad) {
						vid_putchar(padchar);
					}
				}
				vid_printu(u,base);
				break;
			/* string */
			case 's':
				s = va_arg(ap, s8*);
				if(pad > 0) {
					width = vid_getswidth(s);
					while(width++ < pad) {
						vid_putchar(padchar);
					}
				}
				vid_puts(s);
				break;
			/* character */
			case 'c':
				b = (s8)va_arg(ap, u32);
				vid_putchar(b);
				break;
			/* all other */
			default:
				vid_putchar(c);
				break;
		}
		
		/* restore color */
		color = oldcolor;
	}
}

