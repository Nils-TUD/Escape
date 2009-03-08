/**
 * @version		$Id: video.c 95 2008-11-24 18:16:31Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/video.h"
#if IN_KERNEL
#	include "../../kernel/h/common.h"
#	include "../../kernel/h/util.h"
#else
#	include <common.h>
#	include <mem.h>
#	include <io.h>
#endif
#include <stdarg.h>

#define COL_WOB		0x07				/* white on black */
#define COLS		80
#define ROWS		25
#define TAB_WIDTH	2

/* we need some way to share the current position between the kernel and the console-service.
 * therefore we store the position behind the video-mem (seems like that's ok :)). */
#define SET_POS(x) (*(u32*)(videoBase + COLS * (ROWS + 2) * 2) = (x))
#define GET_POS() (*(u32*)(videoBase + COLS * (ROWS + 2) * 2))

/**
 * Handles a color-code
 *
 * @param str a pointer to the current string-position (will be changed)
 */
static void vid_handleColorCode(cstring *str);

#if IN_KERNEL
static void vid_removeBIOSCursor(void);
#endif

/**
 * Sames as vid_printu() but with lowercase letters
 *
 * @param n the number
 * @param base the base
 */
static void vid_printuSmall(u32 n,u8 base);

static u32 videoBase = 0;
static s8 hexCharsBig[] = "0123456789ABCDEF";
static s8 hexCharsSmall[] = "0123456789abcdef";
static u8 color = 0;
static u8 oldBG = 0, oldFG = 0;

/* escape-code-color to video-color */
static u8 colorTable[] = {
	BLACK,
	RED,
	GREEN,
	ORANGE /* should be brown */,
	BLUE,
	MARGENTA,
	CYAN,
	GRAY
};

void vid_init(void) {
#if IN_KERNEL
	videoBase = 0xC00B8000;
	SET_POS(0);
	vid_removeBIOSCursor();
	vid_clearScreen();
	vid_setFGColor(WHITE);
	vid_setBGColor(BLACK);
#else
	videoBase = (u32)mapPhysical(0xB8000,COLS * (ROWS + 2) * 2 + 1);
	if(videoBase == 0)
		printLastError();
	else {
		vid_setFGColor(WHITE);
		vid_setBGColor(BLACK);

		/* request io-ports for qemu and bochs */
		requestIOPort(0xe9);
		requestIOPort(0x3f8);
		requestIOPort(0x3fd);
	}
#endif
}

void vid_clearScreen(void) {
	u8 *addr = (u8*)videoBase;
	u8 *end = (u8*)((u32)addr + COLS * 2 * ROWS);
	for(; addr < end; addr++) {
		*addr = 0;
	}
}

void vid_useColor(eColor bg,eColor fg) {
	oldBG = vid_getBGColor();
	oldFG = vid_getFGColor();
	vid_setBGColor(bg);
	vid_setFGColor(fg);
}

void vid_restoreColor(void) {
	vid_setBGColor(oldBG);
	vid_setFGColor(oldFG);
}

eColor vid_getFGColor(void) {
	return color & 0xF;
}

eColor vid_getBGColor(void) {
	return (color >> 4) & 0xF;
}

void vid_setFGColor(eColor col) {
	color = (color & 0xF0) | (col & 0xF);
}

void vid_setBGColor(eColor col) {
	color = (color & 0x0F) | ((col << 4) & 0xF0);
}

void vid_setLineBG(u8 line,eColor bg) {
	u8 col = ((bg << 4) & 0xF0) | color;
	u8 *addr = (u8*)(videoBase + line * COLS * 2);
	u8 *end = (u8*)((u32)addr + COLS * 2);
	for(addr++; addr < end; addr += 2) {
		*addr = col;
	}
}

u8 vid_getLine(void) {
	s8 *video = (s8*)(videoBase + GET_POS());
	return ((u32)video - videoBase) / (COLS * 2);
}

void vid_toLineEnd(u8 pad) {
	u32 pos = GET_POS();
	u16 col = pos % (COLS * 2);
	SET_POS(pos + ((COLS * 2) - col) - pad * 2);
}

/**
 * Moves all lines one line up, if necessary
 */
static void vid_move(void) {
	u32 i;
	s8 *src,*dst;
	s8 *video = (s8*)(videoBase + GET_POS());
	/* last line? */
	if(video >= (s8*)(videoBase + (ROWS - 1) * COLS * 2)) {
		/* copy all chars one line back */
		src = (s8*)(videoBase + COLS * 2);
		dst = (s8*)videoBase;
		for(i = 0; i < ROWS * COLS * 2; i++) {
			*dst++ = *src++;
		}
		/* to prev line */
		SET_POS(GET_POS() - COLS * 2);
	}
}

void vid_putchar(s8 c) {
	u32 i;
	vid_move();
	s8 *video = (s8*)(videoBase + GET_POS());

	/* write to bochs/qemu console (\r not necessary here) */
	if(c != '\r') {
		outb(0xe9,c);
	    outb(0x3f8,c);
	    while((inb(0x3fd) & 0x20) == 0);
	}

	if(c == '\n') {
		/* to next line */
		SET_POS(GET_POS() + COLS * 2);
		/* move cursor to line start */
		vid_putchar('\r');
	}
	else if(c == '\r') {
		/* to line-start */
		u32 pos = GET_POS();
		SET_POS(pos - (pos % (COLS * 2)));
	}
	else if(c == '\t') {
		i = TAB_WIDTH;
		while(i-- > 0) {
			vid_putchar(' ');
		}
	}
	else {
		u32 pos = GET_POS();
		*video = c;
		video++;
		*video = color;
		/* do an explicit newline if necessary */
		if(((u32)(video - videoBase) % (COLS * 2)) == COLS * 2 - 1)
			vid_putchar('\n');
		else {
			video++;
			SET_POS(pos + 2);
		}
	}
}

void vid_printu(u32 n,u8 base) {
	if(n >= base) {
		vid_printu(n / base,base);
	}
	vid_putchar(hexCharsBig[(n % base)]);
}

u8 vid_getuwidth(u32 n,u8 base) {
	u8 width = 1;
	while(n >= base) {
		n /= base;
		width++;
	}
	return width;
}

void vid_puts(cstring str) {
	s8 c;
	while((c = *str)) {
		/* color-code? */
		if(c == '\033' || c == '\e') {
			if(*(str + 1) == '[') {
				str += 2;
				vid_handleColorCode(&str);
				continue;
			}
		}

		vid_putchar(c);
		str++;
	}
}

u8 vid_getswidth(cstring str) {
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

void vid_printf(cstring fmt,...) {
	va_list ap;
	va_start(ap, fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
}

void vid_vprintf(cstring fmt,va_list ap) {
	s8 c,b,pad,padchar;
	string s;
	s32 n;
	u32 u;
	u8 width,base;

	while (1) {
		/* wait for a '%' */
		while ((c = *fmt++) != '%') {
			/* color-code? */
			if(c == '\033' || c == '\e') {
				if(*fmt == '[') {
					fmt++;
					vid_handleColorCode(&fmt);
					continue;
				}
			}

			/* finished? */
			if (c == '\0') {
				return;
			}
			vid_putchar(c);
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
			case 'X':
				u = va_arg(ap, u32);
				base = c == 'o' ? 8 : (c == 'x' ? 16 : (c == 'b' ? 2 : 10));
				if(pad > 0) {
					width = vid_getuwidth(u,base);
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
				s = va_arg(ap, string);
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
	}
}

static void vid_handleColorCode(cstring *str) {
	cstring fmt = *str;
	while(1) {
		/* read code */
		u8 colCode = 0;
		while(*fmt >= '0' && *fmt <= '9') {
			colCode = colCode * 10 + (*fmt - '0');
			fmt++;
		}

		switch(colCode) {
			/* reset all */
			case 0:
				vid_setFGColor(WHITE);
				vid_setBGColor(BLACK);
				break;

			/* foreground */
			case 30 ... 37:
				colCode = colorTable[colCode - 30];
				vid_setFGColor(colCode);
				break;

			/* default foreground */
			case 39:
				vid_setFGColor(WHITE);
				break;

			/* background */
			case 40 ... 47:
				colCode = colorTable[colCode - 40];
				vid_setBGColor(colCode);
				break;

			/* default background */
			case 49:
				vid_setBGColor(BLACK);
				break;
		}

		/* end of code? */
		if(*fmt == 'm') {
			fmt++;
			break;
		}
		/* we should stop on \0, too */
		if(*fmt == '\0')
			break;
		/* otherwise skip ';' */
		fmt++;
	}

	*str = fmt;
}

#if IN_KERNEL
static void vid_removeBIOSCursor(void) {
	outb(0x3D4,14);
	outb(0x3D5,0x07);
	outb(0x3D4,15);
	outb(0x3D5,0xd0);
}
#endif

static void vid_printuSmall(u32 n,u8 base) {
	if(n >= base) {
		vid_printuSmall(n / base,base);
	}
	vid_putchar(hexCharsSmall[(n % base)]);
}
