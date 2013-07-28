/*
 * Copyright (C) 2013, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <sys/common.h>
#include <sys/dbg/kb.h>
#include <sys/config.h>
#include <sys/video.h>
#include <sys/log.h>
#include <esc/esccodes.h>
#include <assert.h>

fPrintc Video::printFunc = putchar;
ulong Video::col = 0;
ulong Video::row = 0;
uchar Video::color = (BLACK << 4) | WHITE;
ulong Video::targets = SCREEN | LOG;
bool Video::lastWasLineStart = true;
klock_t Video::lock;

void Video::backup(char *buffer,ushort *r,ushort *c) {
	spinlock_aquire(&lock);
	copyScrToMem(buffer,screen(),VID_ROWS);
	*r = row;
	*c = col;
	spinlock_release(&lock);
}

void Video::restore(const char *buffer,ushort r,ushort c) {
	spinlock_aquire(&lock);
	copyMemToScr(screen(),buffer,VID_ROWS);
	row = r;
	col = c;
	spinlock_release(&lock);
}

void Video::printf(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	Video::vprintf(fmt,ap);
	va_end(ap);
}

void Video::vprintf(const char *fmt,va_list ap) {
	if(targets & SCREEN) {
		spinlock_aquire(&lock);
		sPrintEnv env;
		env.print = printFunc;
		env.escape = handleColorCode;
		env.pipePad = handlePipePad;
		env.lineStart = lastWasLineStart;
		prf_vprintf(&env,fmt,ap);
		lastWasLineStart = env.lineStart;
		spinlock_release(&lock);
	}
	if(targets & LOG)
		Log::vprintf(fmt,ap);
}

void Video::putchar(char c) {
	size_t i;
	/* do an explicit newline if necessary */
	if(col >= VID_COLS) {
		if(Config::get(Config::LINEBYLINE))
			Keyboard::get(NULL,KEV_PRESS,true);
		row++;
		col = 0;
	}
	move();

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
			putchar(' ');
	}
	else {
		drawChar(col,row,c);
		col++;
	}
}

void Video::handleColorCode(const char **str) {
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
