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

#include <sys/common.h>
#include <sys/machine/serial.h>
#include <sys/util.h>
#include <sys/log.h>
#include <sys/printf.h>
#include <sys/video.h>
#include <esc/esccodes.h>
#include <string.h>
#include <stdarg.h>

#define VIDEO_BASE			0xC00B8000
#define TAB_WIDTH			4

static void vid_move(void);
static u8 vid_handlePipePad(void);
static void vid_handleColorCode(const char **str);
static void vid_removeBIOSCursor(void);

static fPrintc printFunc = vid_putchar;
static u16 col = 0;
static u16 row = 0;
static u8 color = 0;
static u8 targets = TARGET_SCREEN | TARGET_LOG;

void vid_init(void) {
	vid_removeBIOSCursor();
	vid_clearScreen();
	color = (BLACK << 4) | WHITE;
}

void vid_backup(char *buffer,u16 *r,u16 *c) {
	memcpy(buffer,(void*)VIDEO_BASE,VID_ROWS * VID_COLS * 2);
	*r = row;
	*c = col;
}

void vid_restore(const char *buffer,u16 r,u16 c) {
	memcpy((void*)VIDEO_BASE,buffer,VID_ROWS * VID_COLS * 2);
	row = r;
	col = c;
}

void vid_setTargets(u8 ntargets) {
	targets = ntargets;
}

void vid_clearScreen(void) {
	memclear((void*)VIDEO_BASE,VID_COLS * 2 * VID_ROWS);
	col = row = 0;
}

void vid_setPrintFunc(fPrintc func) {
	printFunc = func;
}

void vid_unsetPrintFunc(void) {
	printFunc = vid_putchar;
}

void vid_putchar(char c) {
	u32 i;
	char *video;
	/* do an explicit newline if necessary */
	if(col >= VID_COLS) {
		row++;
		col = 0;
	}
	vid_move();
	video = (char*)(VIDEO_BASE + row * VID_COLS * 2 + col * 2);


	if(c == '\n') {
		row++;
		col = 0;
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

		col++;
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
	env.print = printFunc;
	env.escape = vid_handleColorCode;
	env.pipePad = vid_handlePipePad;
	if(targets & TARGET_SCREEN)
		prf_vprintf(&env,fmt,ap);
	if(targets & TARGET_LOG)
		log_vprintf(fmt,ap);
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
