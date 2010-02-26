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
#include <log.h>
#include <printf.h>

#define VIDEO_BASE			0xC00B8000
#define TAB_WIDTH			4

static void vid_clearScreen(void);
static void vid_move(void);
static u8 vid_handlePipePad(void);
static void vid_handleColorCode(const char **str);
static void vid_removeBIOSCursor(void);

static u16 col = 0;
static u16 row = 0;
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
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
}

void vid_vprintf(const char *fmt,va_list ap) {
	sPrintEnv env;
	env.print = vid_putchar;
	env.escape = vid_handleColorCode;
	env.pipePad = vid_handlePipePad;
	prf_vprintf(&env,fmt,ap);
	log_vprintf(fmt,ap);
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

static u8 vid_handlePipePad(void) {
	return VID_COLS - col;
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
