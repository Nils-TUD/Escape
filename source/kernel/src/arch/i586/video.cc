/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/arch/i586/serial.h>
#include <sys/arch/i586/ports.h>
#include <sys/dbg/kb.h>
#include <sys/config.h>
#include <sys/log.h>
#include <sys/spinlock.h>
#include <sys/util.h>
#include <sys/printf.h>
#include <sys/video.h>
#include <esc/esccodes.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#define VIDEO_BASE			0xC00B8000
#define TAB_WIDTH			4

static void vid_putchar(char c);
static void vid_move(void);
static uchar vid_handlePipePad(void);
static void vid_handleColorCode(const char **str);
static void vid_removeBIOSCursor(void);

static fPrintc printFunc = vid_putchar;
static ushort col = 0;
static ushort row = 0;
static uchar color = (BLACK << 4) | WHITE;
static uint targets = TARGET_SCREEN | TARGET_LOG;
static klock_t vidLock;
static bool lastWasLineStart = true;

void vid_init(void) {
	vid_removeBIOSCursor();
	vid_clearScreen();
}

void vid_goto(ushort r,ushort c) {
	assert(r < VID_ROWS && c < VID_COLS);
	col = c;
	row = r;
}

void vid_backup(char *buffer,ushort *r,ushort *c) {
	memcpy(buffer,(void*)VIDEO_BASE,VID_ROWS * VID_COLS * 2);
	*r = row;
	*c = col;
}

void vid_restore(const char *buffer,ushort r,ushort c) {
	spinlock_aquire(&vidLock);
	memcpy((void*)VIDEO_BASE,buffer,VID_ROWS * VID_COLS * 2);
	row = r;
	col = c;
	spinlock_release(&vidLock);
}

void vid_setTargets(uint ntargets) {
	targets = ntargets;
}

void vid_clearScreen(void) {
	spinlock_aquire(&vidLock);
	memclear((void*)VIDEO_BASE,VID_COLS * 2 * VID_ROWS);
	col = row = 0;
	spinlock_release(&vidLock);
}

void vid_setPrintFunc(fPrintc func) {
	printFunc = func;
}

void vid_unsetPrintFunc(void) {
	printFunc = vid_putchar;
}

void vid_printf(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
}

void vid_vprintf(const char *fmt,va_list ap) {
	if(targets & TARGET_SCREEN) {
		spinlock_aquire(&vidLock);
		sPrintEnv env;
		env.print = printFunc;
		env.escape = vid_handleColorCode;
		env.pipePad = vid_handlePipePad;
		env.lineStart = lastWasLineStart;
		prf_vprintf(&env,fmt,ap);
		lastWasLineStart = env.lineStart;
		spinlock_release(&vidLock);
	}
	if(targets & TARGET_LOG)
		log_vprintf(fmt,ap);
}

static void vid_putchar(char c) {
	size_t i;
	char *video;
	/* do an explicit newline if necessary */
	if(col >= VID_COLS) {
		if(Config::get(Config::LINEBYLINE))
			Keyboard::get(NULL,KEV_PRESS,true);
		row++;
		col = 0;
	}
	vid_move();
	video = (char*)(VIDEO_BASE + row * VID_COLS * 2 + col * 2);


	if(c == '\n') {
		if(Config::get(Config::LINEBYLINE))
			Keyboard::get(NULL,KEV_PRESS,true);
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

static void vid_move(void) {
	/* last line? */
	if(row >= VID_ROWS) {
		/* copy all chars one line back */
		memmove((void*)VIDEO_BASE,(void*)(VIDEO_BASE + VID_COLS * 2),(VID_ROWS - 1) * VID_COLS * 2);
		memclear((void*)(VIDEO_BASE + (VID_ROWS - 1) * VID_COLS * 2),VID_COLS * 2);
		row--;
	}
}

static uchar vid_handlePipePad(void) {
	return VID_COLS - col;
}

static void vid_handleColorCode(const char **str) {
	int n1,n2,n3;
	int cmd = escc_get(str,&n1,&n2,&n3);
	switch(cmd) {
		case ESCC_COLOR: {
			uchar fg = n1 == ESCC_ARG_UNUSED ? WHITE : MIN(9,n1);
			uchar bg = n2 == ESCC_ARG_UNUSED ? BLACK : MIN(9,n2);
			color = (bg << 4) | fg;
		}
		break;
		default:
			vassert(false,"Invalid escape-code ^[%d;%d,%d,%d]",cmd,n1,n2,n3);
			break;
	}
}

static void vid_removeBIOSCursor(void) {
	ports_outByte(0x3D4,14);
	ports_outByte(0x3D5,0x07);
	ports_outByte(0x3D4,15);
	ports_outByte(0x3D5,0xd0);
}
